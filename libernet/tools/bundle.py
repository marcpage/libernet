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


def largest_files(description):
    """get list of files sorted largest to smallest"""
    return sorted(
        description["files"],
        key=lambda f: description["files"][f]["size"],
        reverse=True,
    )


def reduce_description(description, extracted_files, remove_one_more=False):
    """reduce bundle down to fit within 1 MiB"""
    files = largest_files(description)

    while True:
        contents = serialize_bundle(description)

        if len(contents) <= libernet.tools.block.BLOCK_SIZE:
            break

        next_file = files.pop(0)
        extracted_files[next_file] = description["files"][next_file]
        del description["files"][next_file]

    if files and remove_one_more and extracted_files:
        # we need room for adding sub bundle(s)
        next_file = files.pop(0)
        extracted_files[next_file] = description["files"][next_file]
        del description["files"][next_file]


def finalize_bundle(description, storage):
    """create sub-bundles if necessary to get bundle down to 1 MiB"""
    urls = []
    extracted_files = {}
    reduce_description(description, extracted_files, remove_one_more=True)

    if extracted_files:
        description["bundles"] = []

    while extracted_files:
        sub_description = {"files": extracted_files}
        extracted_files = {}
        reduce_description(sub_description, extracted_files)
        assert len(sub_description["files"]) > 0
        contents = serialize_bundle(sub_description)
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
    """get all the files from the previous version if all the (sub)bundles are available"""
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

        elif enforce:
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


def create(source_path, storage, url=None, max_threads=2):
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
            process_files, file_queue, results_queue, source_path, storage, previous
        )
        for _ in range(0, max_threads)
    ]

    for relative_path in find_all_relative_paths(source_path):
        file_queue.put(relative_path)

    file_queue.put(None)

    for thread in threads:
        thread.join()

    while results_queue.qsize() > 0:
        relative_path, file_description, add_urls = results_queue.get()
        urls.extend(add_urls)
        description["files"][relative_path] = file_description

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

    return missing


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
            return None

        # pylint: disable=E1136
        if path in self.__description["files"]:
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
                    return True

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
                    raise ValueError(
                        f"Block is not the correct size {len(block_contents)} != {block['size']}"
                    )

                destination_file.write(block_contents)

    def missing_blocks(self, path=None):
        """Gets list of all known blocks needed to restore the file
        if self.__path and path is None then missing blocks for all files is returned
        otherwise missing blocks needed to restore path (or if None, self.__path)
        The missing blocks may not be the complete set needed
        """
        path = self.__path if path is None else path
        path_found = self.__ensure_description(path)

        if path_found is None:  # we don't even have the block
            return [f"/sha256/{self.__identifier}"]

        if not path_found:  # we have a bundle but haven't found the file in it
            # pylint: disable=E1136
            missing = self.__description.get("bundles", None)

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
        self.__ensure_description()
        path = path if path is not None else self.__path
        # pylint: disable=E1136
        files = self.__description["files"] if path is None else [path]

        for file in files:
            self.__restore_file(file, destination_root)
