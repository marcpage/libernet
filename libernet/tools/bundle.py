#!/usr/bin/env python3

""" Libernet bundle creator
"""

import os
import json

import libernet.tools.block
import libernet.plat.dirs


def process_file(source_path, storage, relative_path, previous, urls):
    """process a single file"""
    full_path = os.path.join(source_path, relative_path)
    current_file_size = os.path.getsize(full_path)
    current_modified = os.path.getmtime(full_path)

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
        return previous["files"][relative_path]

    description = {
        "size": os.path.getsize(full_path),
        "modified": os.path.getmtime(full_path),
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

    return description


def create(source_path, storage, url=None):
    """Create or update a bundle from the contents of a directory"""
    if url is None:
        previous = {"files": {}}
        description = {"files": {}}
    else:
        previous_text = libernet.tools.block.retrieve_block(url, storage)
        previous = (
            json.loads(previous_text.decode("utf-8"))
            if previous_text
            else {"files": {}}
        )
        description = {"files": {}, "previous": url}

    urls = []

    for root, _, files in os.walk(source_path):
        relative_paths = [
            libernet.plat.dirs.path_relative_to(os.path.join(root, f), source_path)
            for f in files
        ]

        for relative_path in relative_paths:
            file_description = process_file(
                source_path, storage, relative_path, previous, urls
            )
            description["files"][relative_path] = file_description

    contents = json.dumps(description, sort_keys=True, separators=(",", ":"))
    base = libernet.tools.block.store_block(contents.encode("utf-8"), storage)
    return [base, *urls]
