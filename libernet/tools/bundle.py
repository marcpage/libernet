#!/usr/bin/env python3

""" Libernet bundle creator
"""

import os
import json
import threading
import queue

import libernet.tools.block
import libernet.plat.dirs


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
    current_modified = os.path.getmtime(full_path)
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


def load_raw_from_id(identifier, key, storage):
    """ load the raw contents of the top-level bundle description from the identifier """
    bundle_text = libernet.tools.block.get_contents(storage, identifier, key)
    return None if bundle_text is None else json.loads(bundle_text.decode("utf-8"))


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

    sub_urls = finalize_bundle(description, storage)
    return [*sub_urls, *urls]


def missing_blocks(url, storage):
    """determine the blocks needed to fully restore this bundle"""
    bundle = load_raw(url, storage)

    if bundle is None:
        return [url]

    missing = []

    for file in bundle["files"]:
        for part in bundle["files"][file]["parts"]:
            exists = libernet.tools.block.retrieve(part["url"], storage, load=False)

            if not exists:
                missing.append(part["url"])

    for subbundle in bundle.get("bundles", []):
        missing.extend(missing_blocks(subbundle, storage))

    return missing


def restore_file(destination_path, file_description, storage):
    """restore a single file to a given path"""
    libernet.plat.dirs.make_dirs(os.path.split(destination_path)[0])

    with open(destination_path, "wb") as destination_file:
        for block in file_description["parts"]:
            block_contents = libernet.tools.block.retrieve(block["url"], storage)

            if block_contents is None:
                raise FileNotFoundError(f"Block not found: {block['url']}")

            if len(block_contents) != block["size"]:
                raise ValueError(
                    f"Block is not the correct size {len(block_contents)} != {block['size']}"
                )

            destination_file.write(block_contents)


def restore(url, destination, storage):
    """restore an entire bundle into a directory"""
    bundle = get_files(url, storage, enforce=True)

    if bundle is None:
        raise FileNotFoundError("Bundle or sub-bundle url not found")

    # pylint: disable=E1136
    for file in bundle["files"]:
        # pylint: disable=E1136
        restore_file(os.path.join(destination, file), bundle["files"][file], storage)


class Path:
    def __init__(self, url, storage):
        self.__storage = storage
        identifier, key, path = libernet.tools.block.validate_url(url)
        self.__identifier = identifier
        self.__key = key
        self.__path = path
        self.__description = None

    def __ensure_description(self, path=None):
        pass

    def missing_blocks(self, path=None):
        path = path if path is not None else self.__path

        if self.__description is None:
            self.__description = libernet.tools.block.get_contents(self.__storage, self.__identifier, self.__key)

        if self.__description is None:
            return [f'/sha256/{self.__identifier}/aes256/{self.__key}']

        # TODO if path is None, get all files

        if path not in self.__description['files']:
            bundles = list(self.__description.get('bundles', []))
            missing_bundles = []

            for bundle in bundles:
                sub_description = libernet.tools.block.get_contents(self.__storage, self.__identifier, self.__key)

                if sub_description is not None:
                    self.__description['files'].update(sub_description['files'])
                    self.__description['bundles'].remove(bundle)

                    if path in self.__description['files']:
                        break

                else:
                    missing_bundles.append(bundle)

        if path not in self.__description['files']:
            # if there are bundles missing, we need those, else file not found
            return missing_bundles if missing_bundles else None

        files = self.__description['files'] if path is None else [path]
        missing = []

        for file in files:
            urls = [p['url'] for p in self.__description['files'][file]['parts']]
            missing.extend([u for u in urls if libernet.tools.block.retrieve(u, self.__storage, load=False)])

        return missing


    def missing_blocks(self, path=None):
        path = path if path is not None else self.__path

        if self.__description is None:
            self.__description = libernet.tools.block.get_contents(self.__storage, self.__identifier, self.__key)

            if self.__description is None:
                return [f'/sha256/{self.__identifier}/aes256/{self.__key}']

            # TODO if path is None, get all files

            if not path in self.__description['files']:
                bundles = list(self.__description.get('files', []))

                for bundle in bundles:
                    sub_description = libernet.tools.block.get_contents(self.__storage, self.__identifier, self.__key)



