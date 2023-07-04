#!/usr/bin/env python3


""" Bundle of files
"""


import zlib
import json
import stat
import os
import time
import datetime

from libernet.encrypt import aes_encrypt
from libernet.hash import sha256_data_identifier, binary_from_identifier

MAX_BLOCK_SIZE = 1024 * 1024
MAX_BUNDLE_SIZE = MAX_BLOCK_SIZE
UTC_TIMEZONE = datetime.timezone(datetime.timedelta(0))
TIMESTAMP_EPOCH = datetime.datetime(2001, 1, 1, tzinfo=UTC_TIMEZONE).timestamp()


def __create_timestamp(py_time=None):
    """Creates a timestamp (optionally from a python-epoch based time"""
    py_time = time.time() if py_time is None else py_time
    return py_time - TIMESTAMP_EPOCH


def __convert_timestamp(timestamp):
    """Convert a timestamp back into python-epoch based time"""
    return timestamp + TIMESTAMP_EPOCH


def __path_relative_to(path, base):
    """Given path which resides in base, return portion of path relative to base
    returns relative path from base to path
    """
    parts = []
    base = base.rstrip("/")
    while path != base:
        (path, name) = os.path.split(path)
        parts.append(name)
    return os.path.join(*reversed(parts)) if len(parts) > 0 else ""


def __file_metadata_entry(full_path: str) -> dict:
    file_info = os.lstat(full_path)
    is_link = stat.S_ISLNK(file_info.st_mode)

    if is_link:
        file_info = os.stat(full_path)

    assert not stat.S_ISDIR(file_info.st_mode), f"{full_path} is a directory"
    is_readonly = (file_info.st_mode & stat.S_IWUSR) == 0
    is_executable = (file_info.st_mode & stat.S_IXUSR) == stat.S_IXUSR
    description = {
        "size": file_info.st_size,
        "modified": __create_timestamp(file_info.st_mtime),
        "parts": [],
    }

    if is_link:
        description["link"] = os.readlink(full_path)

    if is_readonly:
        description["readonly"] = True

    if is_executable:
        description["executable"] = True

    return description


def __final_block(data: bytes, encrypt=True) -> (str, bytes):
    """gets the url and data for a block"""
    assert len(data) <= MAX_BLOCK_SIZE
    compressed = zlib.compress(data, 9)
    contents_identifier = sha256_data_identifier(data)

    if len(data) < len(compressed):
        compressed = data

    if encrypt:
        key = binary_from_identifier(contents_identifier)
        block_contents = aes_encrypt(key, compressed)
        identifier = sha256_data_identifier(block_contents)
        url = f"/sha256/{identifier}/aes256/{contents_identifier}"
        return (url, block_contents)

    return (f"/sha256/{identifier}", compressed)


def __file_contents(full_path: str, block_queue) -> dict:
    """given the path to a file, create the bundle entry"""
    description = {"contents": []}

    with open(full_path, "rb") as source_file:
        while True:
            block = source_file.read(MAX_BLOCK_SIZE)

            if not block:
                break

            url, data = __final_block(block)
            block_queue.put((url, data))
            description["parts"].append({"url": url, "size": len(data)})


def __serialize_bundle(description):
    """compact serialize a bundle"""
    return json.dumps(description, sort_keys=True, separators=(",", ":")).encode(
        "utf-8"
    )


def __deserialize_bundle(block):
    """compact serialize a bundle"""
    return json.loads(block.decode("utf-8"))


def __file_entry(
    file_path: str, source_path: str, previous: dict, block_queue
) -> (str, dict):
    relative_path = __path_relative_to(file_path, source_path)
    prexisting = previous["files"].get(relative_path, None) if previous else None
    entry = __file_metadata_entry(file_path)
    size_matches = prexisting and prexisting["size"] == entry["size"]
    unmodified = (
        size_matches and abs(prexisting["modified"] - entry["modified"]) < 0.001
    )
    entry.update(
        {"contents": prexisting["contents"]}
        if unmodified
        else __file_contents(file_path, block_queue)
    )
    return relative_path, entry


def __create_raw_bundle(source_path: str, block_queue, previous: dict) -> dict:
    """Given a path to a directory, create a full, raw bundle"""
    description = {"files": {}}
    directories = []
    empty_dirs = []

    for root, dirs, files in os.walk(source_path):
        directories.extend(
            __path_relative_to(os.path.join(root, d), source_path) for d in dirs
        )

        for file in files:
            file_path = os.path.join(root, file)
            description["files"].update(
                dict([__file_entry(file_path, source_path, previous, block_queue)])
            )

    for directory in directories:
        if not any((f.startswith(directory) for f in description["files"])):
            empty_dirs.append(directory)

    description["directories"] = {
        d: os.readlink(os.path.join(source_path, d))
        if os.path.islink(os.path.join(source_path, d))
        else {}
        for d in empty_dirs
    }
    return description


def __files_for_bundle(overhead: int, size_per_block: float, files: dict) -> list:
    filenames = sorted(files, key=lambda f: len(files["contents"]), reverse=True)
    bundle_blocks = 0
    bundle_names = []

    for name in filenames:  # most blocks to least blocks
        additional_blocks = len(files[name]["contents"])
        blocks_size = (bundle_blocks + additional_blocks) * size_per_block
        new_size = blocks_size + overhead

        if new_size >= MAX_BUNDLE_SIZE:
            break

        bundle_blocks += additional_blocks
        bundle_names.append(name)

    return bundle_names


def __thin_bundle(raw: dict, block_queue) -> str:
    raw["bundles"] = []
    bundle = __serialize_bundle(raw)
    files_size = len(__serialize_bundle(raw["files"]))
    size_per_block = files_size / sum(
        len(raw["files"][f].get("contents", []) for f in raw["files"])
    )
    subbundle_count = ((len(bundle) - 1) // MAX_BUNDLE_SIZE + 1) - 1
    overhead = len(bundle) - files_size + int(subbundle_count * size_per_block)
    files = raw["files"]
    bundle_filenames = __files_for_bundle(overhead, size_per_block, files)
    raw["files"] = {f: files[f] for f in bundle_filenames}
    bundle = __serialize_bundle(raw)
    assert len(bundle) <= MAX_BUNDLE_SIZE, len(bundle)

    for file in bundle_filenames:
        del files[file]

    while len(files) > 0:
        subraw = {"files": {}}
        overhead = len(__serialize_bundle(subraw))
        bundle_filenames = __files_for_bundle(overhead, size_per_block, files)
        subraw["files"] = {f: files[f] for f in bundle_filenames}
        subbundle = __serialize_bundle(subraw)
        assert len(subbundle) <= MAX_BUNDLE_SIZE, len(subbundle)
        url, data = __final_block(subbundle)
        block_queue.put((url, data))
        raw["bundles"].append(url)
        bundle = __serialize_bundle(raw)
        assert len(bundle) <= MAX_BUNDLE_SIZE, len(bundle)

        for file in bundle_filenames:
            del files[file]

    url, data = __final_block(bundle)
    block_queue.put((url, data))
    return url


def create(path: str, block_queue, previous: dict = None, **kwargs) -> str:
    """stores a bundle from path in block_queue and returns the url
    block_queue - an object that can be called with put((block_url, block_data))
                    NOTE: the block_url may have decryption key
    previous - a bundle dictionary for optimization, see inflate()
    kwargs - added to the bundle description
    """
    raw = __create_raw_bundle(path, block_queue, previous)
    raw.update(kwargs)
    bundle = __serialize_bundle(raw)

    if len(bundle) > MAX_BUNDLE_SIZE:
        return __thin_bundle(raw, block_queue)

    url, data = __final_block(bundle)
    block_queue.put((url, data))
    return url


def inflate(url: str, storage) -> dict:
    """inflates a bundle from the given url
    storage - an object with get(url:str) -> bytes
    returns as much as could be inflated
    """
    bundle_data = storage.get(url)

    if bundle_data is None:
        return None

    bundle = __deserialize_bundle(bundle_data)
    missing = []

    for suburl in bundle.get("bundles", []):
        bundle_data = storage.get(suburl)

        if bundle_data is None:
            missing.append(suburl)
        else:
            subbundle = __deserialize_bundle(bundle_data)
            bundle["files"].update(subbundle["files"])

    bundle["bundles"] = missing
    return bundle
