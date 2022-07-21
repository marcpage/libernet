#!/usr/bin/env python3

""" Libernet bundle creator
"""

import os
import json
import threading
import queue

import libernet.tools.block
import libernet.plat.dirs
import libernet.plat.timestamp


def start_thread(target, *args, **kwargs):
    """Create a thread running a function with the given parameters."""
    thread = threading.Thread(target=target, args=args, kwargs=kwargs, daemon=True)
    thread.start()
    return thread


def process_file(source_path, storage, relative_path, previous):
    """process a single file"""
    full_path = os.path.join(source_path, relative_path)
    # pylint: disable=W0511
    # TODO use stat to get size and mtime
    current_file_size = os.path.getsize(full_path)
    current_modified = libernet.plat.timestamp.create(os.path.getmtime(full_path))
    urls = []

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
        # pylint: disable=W0511
        # TODO should we return the urls if we didn't update them?
        return (relative_path, previous["files"][relative_path], urls)

    description = {
        "size": current_file_size,
        "modified": current_modified,
        "parts": [],
    }

    with open(full_path, "rb") as source_file:
        while True:
            block = source_file.read(libernet.tools.block.BLOCK_SIZE)

            if not block:
                break

            url = libernet.tools.block.store_block(block, storage)
            urls.append(url)
            description["parts"].append(
                {
                    "url": url,
                    "size": len(block),
                }
            )

    return (relative_path, description, urls)


def serialize_bundle(description):
    """compact serialize a bundle"""
    return json.dumps(description, sort_keys=True, separators=(",", ":"))


def smallest_files(description):
    """get list of files sorted smallest to largest"""
    return sorted(
        description["files"],
        key=lambda f: description["files"][f]["size"],
        reverse=False,
    )


def reduce_description(description, top_level_bundle=False):
    """remove files until we are bundle sized, returns files removed"""
    contents = serialize_bundle(description)

    if len(contents) <= libernet.tools.block.BLOCK_SIZE:
        return {}

    files = smallest_files(description)  # get the smallest to largest file
    average_size_per_file = len(contents) / len(files)
    subbundle_count = (
        int(len(contents) / libernet.tools.block.BLOCK_SIZE) if top_level_bundle else 0
    )
    root_bundle_file_count = int(
        libernet.tools.block.BLOCK_SIZE / average_size_per_file - subbundle_count / 2
    )
    extracted_files = {
        f: description["files"][f] for f in files[root_bundle_file_count:]
    }

    for file in extracted_files:
        del description["files"][file]

    files = smallest_files(description)  # get the smallest to largest file

    while True:  # we rarely need to remove any more files
        contents = serialize_bundle(description)

        if len(contents) <= libernet.tools.block.BLOCK_SIZE:
            break

        to_remove = files.pop()  # NOT TESTED
        extracted_files[to_remove] = description["files"][to_remove]
        del description["files"][to_remove]

    assert len(contents) <= libernet.tools.block.BLOCK_SIZE, (
        f"Unable to reduce description {len(contents)} "
        + f"/ {libernet.tools.block.BLOCK_SIZE} "
        + f"({len(contents) - libernet.tools.block.BLOCK_SIZE} too big)"
    )
    return extracted_files


def finalize_bundle(description, storage):
    """create sub-bundles if necessary to get bundle down to 1 MiB"""
    urls = []
    extracted_files = reduce_description(description, top_level_bundle=True)

    if extracted_files:
        description["bundles"] = []

    while extracted_files:
        sub_description = {"files": extracted_files}
        extracted_files = reduce_description(sub_description, extracted_files)
        assert len(sub_description["files"]) > 0
        contents = serialize_bundle(sub_description)
        assert len(contents) <= libernet.tools.block.BLOCK_SIZE, (
            f"contents is too big {len(contents)} "
            + f"/ {libernet.tools.block.BLOCK_SIZE} "
            + f"({len(contents) - libernet.tools.block.BLOCK_SIZE} too big)"
        )
        url = libernet.tools.block.store_block(contents.encode("utf-8"), storage)
        description["bundles"].append(url)
        urls.append(url)

    top_bundle = serialize_bundle(description)
    top_level = libernet.tools.block.store_block(top_bundle.encode("utf-8"), storage)
    return [top_level, *urls]


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
        if enforce:  # NOT TESTED
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


def find_all_relative_paths(source_path):
    """find all files as relative paths in a directory"""
    all_relative_paths = []

    for root, _, files in os.walk(source_path):
        all_relative_paths.extend(
            [
                libernet.plat.dirs.path_relative_to(os.path.join(root, f), source_path)
                for f in files
            ]
        )

    return all_relative_paths


def process_files(in_queue, out_queue, source_path, storage, previous):
    """thread to process files"""
    while True:
        relative_path = in_queue.get()

        if relative_path is None:
            in_queue.put(None)
            break

        results = process_file(source_path, storage, relative_path, previous)
        out_queue.put(results)


def __queue_all_files(path, file_queue):
    """get all files in a path and queue them with a terminating None"""
    all_files = find_all_relative_paths(path)

    for relative_path in all_files:
        file_queue.put(relative_path)

    file_queue.put(None)


def create(source_path, storage, url=None, max_threads=2, verbose=False):
    """Create or update a bundle from the contents of a directory"""
    if url is None:
        previous = {"files": {}}
        description = {"files": {}}
    else:
        previous = get_files(url, storage)
        description = {"files": {}, "previous": url}

    urls = []
    file_queue = queue.Queue()
    results_queue = queue.Queue()
    threads = [
        start_thread(
            process_files,
            file_queue,
            results_queue,
            source_path,
            storage,
            previous,
        )
        for _ in range(0, max_threads)
    ]
    __queue_all_files(source_path, file_queue)

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

        if verbose:
            print(found[0])  # NOT TESTED

    description["timestamp"] = libernet.plat.timestamp.create()
    sub_urls = finalize_bundle(description, storage)
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

    def __ensure_description(self, path=None):
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
            return None  # NOT TESTED

        # pylint: disable=E1136
        if path in self.__description["files"]:
            return True  # NOT TESTED

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
                    return True  # NOT TESTED

        return path is None

    def __restore_file(self, path, destination_root):
        """restore a single file to a given path"""
        destination_path = os.path.join(destination_root, path)
        # pylint: disable=E1136
        file_description = self.__description["files"][path]
        libernet.plat.dirs.make_dirs(os.path.split(destination_path)[0])

        with open(destination_path, "wb") as destination_file:
            for block in file_description["parts"]:
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

    def relative_path(self, path=None, just_bundle=False):  # NOT TESTED
        """get the relative path to the bundle contents"""
        base = os.path.join(
            "sha256",
            self.__identifier[: libernet.tools.block.BLOCK_TOP_DIR_SIZE],
            self.__identifier,
            "aes256",
            self.__key,
        )

        if just_bundle or path is None and self.__path is None:
            return base

        return os.path.join(base, self.__path if path is None else path)

    def missing_blocks(self, path=None):
        """Gets list of all known blocks needed to restore the file
        if self.__path and path is None then missing blocks for all files is returned
        otherwise missing blocks needed to restore path (or if None, self.__path)
        The missing blocks may not be the complete set needed
        """
        path = self.__path if path is None else path
        path_found = self.__ensure_description(path)

        if path_found is None:  # we don't even have the block
            return [f"/sha256/{self.__identifier}"]  # NOT TESTED

        if not path_found:  # we have a bundle but haven't found the file in it
            # pylint: disable=E1136
            missing = self.__description.get("bundles", None)  # NOT TESTED

            if missing is None:  # there are no more bundles to search
                return None  # file not found

            return list(missing)  # we need other sub-bundles to search

        # pylint: disable=E1136
        files = self.__description["files"] if path is None else [path]
        missing = []

        for file in files:
            # pylint: disable=E1136
            urls = [p["url"] for p in self.__description["files"][file]["parts"]]
            missing.extend(
                [
                    u
                    for u in urls
                    if not libernet.tools.block.retrieve(u, self.__storage, load=False)
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
        found = self.__ensure_description(path)

        if not found:
            return found  # NOT TESTED

        # pylint: disable=E1136
        files = self.__description["files"] if path is None else [path]

        for file in files:
            self.__restore_file(file, destination_root)

        return True
