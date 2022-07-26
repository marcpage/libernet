#!/usr/bin/env python3

""" Libernet bundle creator
"""

import os
import json
import threading
import queue
import stat

import xattr

import libernet.tools.block
import libernet.plat.dirs
import libernet.plat.timestamp
import libernet.plat.files
import libernet.tools.encrypt

MAX_BUNDLE_SIZE = libernet.tools.block.BLOCK_SIZE
BUNDLE_OVERHEAD = len(json.dumps({"bundles": []}))
FILE_OVERHEAD = len(json.dumps({"files": []}))
PER_IDENTIFIER_OVERHEAD = len('", "')
BLOCK_TOP_DIR_SIZE = libernet.tools.block.BLOCK_TOP_DIR_SIZE
IDENTIFIER_LENGTH = libernet.tools.encrypt.IDENTIFIER_LENGTH
URL_LENGTH = IDENTIFIER_LENGTH * 2 + len("/sh256/aes256") + BLOCK_TOP_DIR_SIZE


def __start_thread(target, *args, **kwargs):
    """Create a thread running a function with the given parameters."""
    thread = threading.Thread(target=target, args=args, kwargs=kwargs, daemon=True)
    thread.start()
    return thread


def __get_file_description(full_path, relative_path, previous):
    """Get initial file meta-data"""
    file_info = os.lstat(full_path)
    is_link = stat.S_ISLNK(file_info.st_mode)
    is_readonly = (file_info.st_mode & stat.S_IWUSR) == 0
    is_executable = (file_info.st_mode & stat.S_IXUSR) == stat.S_IXUSR

    if is_link:
        file_info = os.stat(full_path)

    current_file_size = file_info.st_size
    current_modified = libernet.plat.timestamp.create(file_info.st_mtime)

    if relative_path in previous["files"]:
        size_match = current_file_size == previous["files"][relative_path]["size"]
        modified_match = (
            abs(current_modified - previous["files"][relative_path]["modified"])
            < 0.0001
        )
    else:
        size_match = False
        modified_match = False

    if modified_match and size_match:
        return None

    description = {
        "size": current_file_size,
        "modified": current_modified,
        "parts": [],
    }

    if is_link:
        description["link"] = os.readlink(full_path)

    if is_readonly:
        description["readonly"] = True

    if is_executable:
        description["executable"] = True

    return description


def __store_file_parts(path, storage, urls):
    """take parts from a file at given path and store them and return urls"""
    parts = []

    with open(path, "rb") as source_file:
        while True:
            block = source_file.read(MAX_BUNDLE_SIZE)

            if not block:
                break
            url = libernet.tools.block.store_block(block, storage)
            urls.append(url)
            parts.append(
                {
                    "url": url,
                    "size": len(block),
                }
            )

    return parts


def __store_xattr(path, storage):
    """store all xattr properties from a given path and return map of attributes to stored url"""
    return {
        a: libernet.tools.block.store_block(xattr.getxattr(path, a), storage)
        for a in xattr.listxattr(path)
    }


def __process_file(source_path, storage, relative_path, previous):
    """process a single file"""
    full_path = os.path.join(source_path, relative_path)
    description = __get_file_description(full_path, relative_path, previous)
    urls = []

    if description is None:
        # pylint: disable=W0511
        # TODO should we return the urls if we didn't update them?
        return (relative_path, previous["files"][relative_path], urls)

    description["parts"] = __store_file_parts(full_path, storage, urls)
    rsrc_path = libernet.plat.files.rsrc_fork_path(full_path)
    file_attributes = __store_xattr(full_path, storage)

    if rsrc_path is not None:
        description["rsrc"] = __store_file_parts(rsrc_path, storage, urls)

    if file_attributes:
        description["xattr"] = file_attributes

    return (relative_path, description, urls)


def __serialize_bundle(description):
    """compact serialize a bundle"""
    return json.dumps(description, sort_keys=True, separators=(",", ":"))


def __find_files_in_bundle(files, files_by_relative_size):
    """use binary search to find maximum files in each bundle"""
    (low, high) = (1, len(files))

    while True:
        mid = int((high + low) / 2)
        bundle_files = {
            files_by_relative_size[i]: files[files_by_relative_size[i]]
            for i in range(0, mid)
        }
        bundle_size = FILE_OVERHEAD + len(__serialize_bundle(bundle_files))

        if bundle_size > MAX_BUNDLE_SIZE:
            high = mid

        else:
            low = mid

        if (high - low) <= 1 and bundle_size <= MAX_BUNDLE_SIZE:
            break

    for file in bundle_files:
        del files[file]
        files_by_relative_size.remove(file)

    return bundle_files


def __breakdown_bundle(description):
    """Returns the descriptions of all the bundles needed to store, with top-most first"""
    description = dict(description)  # don't modify the incoming description
    contents = __serialize_bundle(description)

    if len(contents) <= MAX_BUNDLE_SIZE:
        return [(description, contents)]

    bundles = []
    files = dict(description["files"])
    description["bundles"] = []
    description["files"] = {}
    relative_file_sizes = {f: len(files[f]["parts"]) for f in files}
    files_by_relative_size = sorted(
        relative_file_sizes, key=lambda f: relative_file_sizes[f], reverse=True
    )

    while len(files) > 0:
        subbundle = {"files": __find_files_in_bundle(files, files_by_relative_size)}
        subcontent = __serialize_bundle(subbundle)
        assert len(subcontent) <= MAX_BUNDLE_SIZE, (
            f"subbundle is too big {len(subcontent)} "
            + f"/ {MAX_BUNDLE_SIZE} "
            + f"({len(subcontent) - MAX_BUNDLE_SIZE} too big)"
        )
        bundles.append((subbundle, subcontent))

    bundles.sort(key=lambda f: len(f[1]))
    first_bundle_size = (
        len(__serialize_bundle(description))
        + len(bundles[0][1])
        + (URL_LENGTH + PER_IDENTIFIER_OVERHEAD) * len(bundles)
    )

    if (
        first_bundle_size < MAX_BUNDLE_SIZE
    ):  # bundle small enough to merge with main bundle
        description.update(bundles.pop(0)[0])

    bundles.insert(0, (description, None))
    return bundles


def __finalize_bundle(description, storage):
    """break a bundle down into mutliple bundles to fit within the size limit"""
    bundles = __breakdown_bundle(description)
    main_bundle = bundles.pop(0)[0]
    urls = []

    for bundle in bundles:
        url = libernet.tools.block.store_block(bundle[1].encode("utf-8"), storage)
        main_bundle["bundles"].append(url)

    if "bundles" in main_bundle:
        urls.extend(main_bundle["bundles"])

    contents = __serialize_bundle(main_bundle)
    assert len(contents) <= MAX_BUNDLE_SIZE, (
        f"contents is too big {len(contents)} "
        + f"/ {MAX_BUNDLE_SIZE} "
        + f"({len(contents) - MAX_BUNDLE_SIZE} too big)"
    )
    urls.insert(0, libernet.tools.block.store_block(contents.encode("utf-8"), storage))
    return urls


def load_raw(url, storage):
    """Get the raw contents of the top-level bundle"""
    bundle_text = libernet.tools.block.retrieve(url, storage)
    return None if bundle_text is None else json.loads(bundle_text.decode("utf-8"))


def get_files(url, storage, enforce=False):
    """get all the files from the previous version if all the (sub)bundles are available
    enforce - if True then return None if we cannot find all files (missing (sub)bundles)
    """
    bundle = load_raw(url, storage)

    if bundle is None:
        if enforce:
            return None

        bundle = {"files": {}}

    for bundle_url in bundle.get("bundles", []):
        sub_bundle = get_files(bundle_url, storage, enforce)

        if sub_bundle is not None:
            # pylint: disable=E1136
            bundle["files"].update(sub_bundle["files"])

        elif enforce:  # NOT TESTED
            return None

    return bundle


def __find_all_relative_paths(source_path):
    """find all files as relative paths in a directory"""
    all_relative_paths = []
    all_relative_dirs = []
    empty_dirs = []

    for root, dirs, files in os.walk(source_path):
        all_relative_paths.extend(
            [
                libernet.plat.dirs.path_relative_to(os.path.join(root, f), source_path)
                for f in files
            ]
        )
        all_relative_dirs.extend(
            [
                libernet.plat.dirs.path_relative_to(os.path.join(root, d), source_path)
                for d in dirs
            ]
        )

    for directory in all_relative_dirs:
        if not any((f.startswith(directory) for f in all_relative_paths)):
            empty_dirs.append(directory)

    return (all_relative_paths, empty_dirs)


def __process_files(in_queue, out_queue, source_path, storage, previous):
    """thread to process files"""
    while True:
        relative_path = in_queue.get()

        if relative_path is None:
            in_queue.put(None)
            break

        results = __process_file(source_path, storage, relative_path, previous)
        out_queue.put(results)


def __queue_all_files(path, file_queue):
    """get all files in a path and queue them with a terminating None"""
    (all_files, empty_dirs) = __find_all_relative_paths(path)

    for relative_path in all_files:
        file_queue.put(relative_path)

    file_queue.put(None)

    return {
        d: (
            {"link": os.readlink(os.path.join(path, d))}
            if os.path.islink(os.path.join(path, d))
            else {}
        )
        for d in empty_dirs
    }


def __add_files_to_description(description, source_path, storage, previous, **kwargs):
    """finds all the files in source_path
    adds them to storage and the description
    returns the urls found
    """
    urls = []
    file_queue = queue.Queue()
    results_queue = queue.Queue()
    threads = [
        __start_thread(
            __process_files,
            file_queue,
            results_queue,
            source_path,
            storage,
            previous,
        )
        for _ in range(0, kwargs.get("max_threads", 2))
    ]
    description["directories"] = __queue_all_files(source_path, file_queue)

    while True:
        if file_queue is not None:
            if file_queue.qsize() == 1:
                file_queue = None

                for thread in threads:
                    thread.join()

        if file_queue is None:
            if results_queue.qsize() == 0:
                break

        found = results_queue.get()
        urls.extend(found[2])
        description["files"][found[0]] = found[1]

        if kwargs.get("verbose", False):
            print(found[0])

    return urls


def create(source_path, storage, max_threads=2, verbose=False, **kwargs):
    """Create or update a bundle from the contents of a directory
    kwargs
        previous - the url of the previous incarnation (speeds up encoding)
        index - the root file to return if no path is specified
        all others are just included in the bundle description
    """
    description = dict(kwargs)
    description["files"] = {}
    previous = (
        get_files(kwargs["previous"], storage)
        if kwargs.get("previous", None)
        else {"files": {}}
    )
    index = kwargs.get("index", None)
    assert index is None or "/" not in index, "index must be a file in the root"
    urls = __add_files_to_description(
        description,
        source_path,
        storage,
        previous,
        max_threads=max_threads,
        verbose=verbose,
    )
    description["timestamp"] = libernet.plat.timestamp.create()

    for key in [k for k, v in description.items() if v is None]:
        del description[key]

    if index is not None and index not in description["files"]:
        raise FileNotFoundError(f"Requested index '{index}' is not in the bundle")

    sub_urls = __finalize_bundle(description, storage)
    return [*sub_urls, *urls]


def missing_blocks(url, storage):
    """determine the blocks needed to fully restore this bundle"""
    bundle = load_raw(url, storage)

    if bundle is None:
        bundle_identifier = libernet.tools.block.validate_url(url)[0]
        return [f"sha256/{bundle_identifier}"]

    missing = []

    for file in bundle["files"]:
        for part in bundle["files"][file]["parts"]:
            exists = libernet.tools.block.retrieve(part["url"], storage, load=False)

            if not exists:
                bundle_identifier = libernet.tools.block.validate_url(part["url"])[0]
                missing.append(f"sha256/{bundle_identifier}")

    for subbundle in bundle.get("bundles", []):
        missing.extend(missing_blocks(subbundle, storage))

    return ["/".join(m.split("/")[:3]) for m in missing]


class Path:
    """Bundle path
    can be either all files in the bundle or just a single file
    """

    def __init__(self, url, storage):
        """create a new bundle path"""
        self.__storage = storage
        identifier, key, path = libernet.tools.block.validate_url(url)
        self.__identifier = identifier
        self.__key = key
        self.__path = path
        self.__description = None

    def __ensure_description(self, path=None, just_load=False):
        """ensure we have a loaded description
        returns None if the bundle was not found
        returns False if the path was not found in the bundle
            (check missing_blocks() to see if we got all the bundles)
        returns True if the file was found
        If path and self.__path are none then all sub bundles are loaded
        otherwise enough sub bundles are loaded to
            find path (or self.__path if path is None)
        """
        path = path if path is not None else self.__path

        if self.__description is None:
            contents = libernet.tools.block.get_contents(
                self.__storage, self.__identifier, self.__key
            )
            self.__description = json.loads(contents) if contents else None

        if self.__description is None:
            return None

        # pylint: disable=E1136
        if path in self.__description["files"]:
            return True

        if path in self.__description.get("directories", []):
            return True

        if just_load:
            return True

        # pylint: disable=E1136
        bundles = list(self.__description.get("bundles", []))

        for bundle in bundles:
            sub_identifier, sub_key, _ = libernet.tools.block.validate_url(bundle)
            sub_description = libernet.tools.block.get_contents(
                self.__storage, sub_identifier, sub_key
            )

            if sub_description is not None:
                sub_bundle = json.loads(sub_description)
                # pylint: disable=E1136
                self.__description["files"].update(sub_bundle["files"])
                # pylint: disable=E1136
                self.__description["bundles"].remove(bundle)
                # pylint: disable=E1136
                self.__description["bundles"].extend(sub_bundle.get("bundles", []))

                # pylint: disable=E1136
                if path in self.__description["files"]:
                    return True  # NOT TESTED check for missing blocks for a file in a bundle

        return path is None

    def __restore_file_contents(self, path, parts):
        with open(path, "wb") as destination_file:
            for block in parts:
                block_contents = libernet.tools.block.retrieve(
                    block["url"], self.__storage
                )

                if block_contents is None:
                    raise FileNotFoundError(f"Block not found: {block['url']}")

                if len(block_contents) != block["size"]:
                    raise ValueError(  # NOT TESTED
                        f"Block is not the correct size {len(block_contents)} != {block['size']}"
                    )

                destination_file.write(block_contents)

    def __restore_file(self, path, destination_root):
        """restore a single file to a given path"""
        destination_path = os.path.join(destination_root, path)
        # pylint: disable=E1136
        file_description = self.__description["files"][path]
        libernet.plat.dirs.make_dirs(os.path.split(destination_path)[0])
        link_contents = file_description.get("link", None)
        is_readonly = file_description.get("readonly", False)
        is_executable = file_description.get("executable", False)

        if link_contents is not None:
            libernet.plat.files.symlink(link_contents, destination_path)
            return

        self.__restore_file_contents(destination_path, file_description["parts"])

        if "rsrc" in file_description:
            self.__restore_file_contents(
                libernet.plat.files.rsrc_fork_path(destination_path, verify=False),
                file_description["rsrc"],
            )

        if "xattr" in file_description:
            for attribute in file_description["xattr"]:
                xattr.setxattr(
                    destination_path,
                    attribute,
                    libernet.tools.block.retrieve(
                        file_description["xattr"][attribute], self.__storage
                    ),
                )

        if is_readonly or is_executable:
            mode = os.stat(destination_path).st_mode
            mode = (mode & ~stat.S_IWUSR) if is_readonly else mode
            mode = mode | (stat.S_IXUSR if is_executable else 0)
            os.chmod(destination_path, mode)

    def relative_path(self, path=None, just_bundle=False):
        """get the relative path to the bundle contents"""
        base = os.path.join(
            "sha256",
            self.__identifier[: libernet.tools.block.BLOCK_TOP_DIR_SIZE],
            self.__identifier,
            "aes256",
            self.__key,
        )

        if just_bundle or path is None and self.__path is None:
            return base  # NOT TESTED

        return os.path.join(base, self.__path if path is None else path)

    def index(self):
        """Get the index file (or None if there is none)"""
        self.__ensure_description(just_load=True)
        return self.__description.get("index", None)

    def missing_blocks(self, path=None):
        """Gets list of all known blocks needed to restore the file
        if self.__path and path is None then missing blocks for all files is returned
        otherwise missing blocks needed to restore path (or if None, self.__path)
        The missing blocks may not be the complete set needed
        """
        path = self.__path if path is None else path

        # NOT TESTED add an index and test for missing blocks on empty path
        if path == "":
            path = self.__description.get("index", None)

            if path is None:
                raise FileNotFoundError(
                    "This bundle has no index, but one was requested"
                )

        path_found = self.__ensure_description(path)

        if path_found is None:  # we don't even have the block
            return [f"/sha256/{self.__identifier}"]

        if path is not None and path in self.__description.get("directories", []):
            return []

        if not path_found:  # we have a bundle but haven't found the file in it
            # pylint: disable=E1136
            missing = self.__description.get("bundles", None)

            if missing is None:  # there are no more bundles to search
                return None  # file not found

            # NOT TESTED  delete subbundle block
            return list(missing)  # we need other sub-bundles to search

        # pylint: disable=E1136
        files = self.__description["files"] if path is None else [path]
        missing = []

        for file in files:
            # pylint: disable=E1136
            urls = [p["url"] for p in self.__description["files"][file]["parts"]]
            rsrc_urls = [
                p["url"] for p in self.__description["files"][file].get("rsrc", [])
            ]
            missing.extend(
                [
                    u
                    for u in urls
                    if not libernet.tools.block.retrieve(u, self.__storage, load=False)
                ]
            )
            missing.extend(
                [
                    u
                    for u in rsrc_urls
                    if not libernet.tools.block.retrieve(u, self.__storage, load=False)
                ]
            )
            missing.extend(
                [
                    self.__description["files"][file]["xattr"][a]
                    for a in self.__description["files"][file].get("xattr", [])
                    if not libernet.tools.block.retrieve(
                        self.__description["files"][file]["xattr"][a],
                        self.__storage,
                        load=False,
                    )
                ]
            )

        return missing

    def restore_file(self, destination_root, path=None):
        """restore file(s) to a destination directory
        file(s) restored are the full relative path under destination_root
        if self.__path and path are None, all files are restored
        otherwise path (or if None, self.__path) is restored
        """
        path = path if path is not None else self.__path

        if (
            path is not None and len(path) == 0
        ):  # NOT TESTED set and index and restore "" and verify the index was created
            index = self.__description.get("index", None)

            if index is not None:
                path = index

        found = self.__ensure_description(path)

        if not found:
            return found

        # pylint: disable=E1136
        files = list(self.__description["files"]) if path is None else [path]

        if path is None:
            files.extend(self.__description.get("directories", []))

        for file in files:
            if file in self.__description["files"]:
                self.__restore_file(file, destination_root)

            elif "link" in self.__description["directories"][file]:
                libernet.plat.files.symlink(
                    self.__description["directories"][file]["link"],
                    os.path.join(destination_root, file),
                )

            else:
                libernet.plat.dirs.make_dirs(os.path.join(destination_root, file))

        return True
