#!/usr/bin/env python3

""" Libernet bundle creator
"""

import os
import json
import multiprocessing

import libernet.tools.block
import libernet.plat.dirs


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

    if remove_one_more and extracted_files:  # we need room for adding sub bundle(s)
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
        contents = serialize_bundle(sub_description)
        url = libernet.tools.block.store_block(contents.encode("utf-8"), storage)
        description["bundles"].append(url)
        urls.append(url)

    top_bundle = serialize_bundle(description)
    top_level = libernet.tools.block.store_block(top_bundle.encode("utf-8"), storage)
    return [top_level, *urls]


def get_previous_files(url, storage):
    """get all the files from the previous version if all the (sub)bundles are available"""
    previous_text = libernet.tools.block.retrieve_block(url, storage)

    if previous_text is None:
        return {"files": {}}

    previous = json.loads(previous_text.decode("utf-8"))

    for bundle_url in previous.get("bundle", []):
        sub_bundle = get_previous_files(bundle_url, storage)
        previous["files"].update(sub_bundle["files"])

    return previous


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


def create(source_path, storage, url=None, max_threads=2):
    """Create or update a bundle from the contents of a directory"""
    if url is None:
        previous = {"files": {}}
        description = {"files": {}}
    else:
        previous = get_previous_files(url, storage)
        description = {"files": {}, "previous": url}

    urls = []
    all_relative_paths = [
        (source_path, storage, p, previous)
        for p in find_all_relative_paths(source_path)
    ]

    with multiprocessing.Pool(max_threads) as pool:
        results = pool.starmap(process_file, all_relative_paths)

    for relative_path, file_description, add_urls in results:
        urls.extend(add_urls)
        description["files"][relative_path] = file_description

    sub_urls = finalize_bundle(description, storage)
    return [*sub_urls, *urls]
