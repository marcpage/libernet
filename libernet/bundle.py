#!/usr/bin/env python3


""" Bundle of files
"""


import json
import stat
import os
import time
import datetime

import libernet.block


MAX_BLOCK_SIZE = 1024 * 1024
MAX_BUNDLE_SIZE = MAX_BLOCK_SIZE
UTC_TIMEZONE = datetime.timezone(datetime.timedelta(0))
TIMESTAMP_EPOCH = datetime.datetime(2001, 1, 1, tzinfo=UTC_TIMEZONE).timestamp()
SAME_TIME_VARIANCE_IN_SECONDS = 0.000100  # 100 microseconds

# keys
BUNDLES = "bundles"
CONTENTS = "contents"
DIRECTORIES = "directories"
EXUTABLE = "executable"
FILES = "files"
LINK = "link"
MODIFIED = "modified"
READONLY = "readonly"
READ_BINARY = "rb"
SIZE = "size"
URL = "url"


def __create_timestamp(py_time=None):
    """Creates a timestamp (optionally from a python-epoch based time"""
    py_time = time.time() if py_time is None else py_time
    return py_time - TIMESTAMP_EPOCH


def __convert_timestamp(timestamp):
    """Convert a timestamp back into python-epoch based time"""
    return timestamp + TIMESTAMP_EPOCH


def __set_mod_time(path: str, timestamp: float):
    """Sets the modification timestamp of a file"""
    file_info = os.stat(path)
    access_time_in_ns = int(file_info.st_atime * 1_000_000_000)
    mod_time_in_ns = int(__convert_timestamp(timestamp) * 1_000_000_000)
    os.utime(path, ns=(access_time_in_ns, mod_time_in_ns))
    file_info = os.stat(path)


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
    """Get the file metadata"""
    file_info = os.lstat(full_path)
    is_link = stat.S_ISLNK(file_info.st_mode)

    if is_link:
        file_info = os.stat(full_path)

    assert not stat.S_ISDIR(file_info.st_mode), f"{full_path} is a directory"
    is_readonly = (file_info.st_mode & stat.S_IWUSR) == 0
    is_executable = (file_info.st_mode & stat.S_IXUSR) == stat.S_IXUSR
    description = {
        SIZE: file_info.st_size,
        MODIFIED: __create_timestamp(file_info.st_mtime),
    }

    if is_link:
        description[LINK] = os.readlink(full_path)

    if is_readonly:
        description[READONLY] = True

    if is_executable:
        description[EXUTABLE] = True

    # TODO: add xattr  # pylint: disable=fixme
    # TODO: add rsrc  # pylint: disable=fixme

    return description


def __file_contents(full_path: str, storage) -> dict:
    """given the path to a file, create the bundle entry"""
    description = {CONTENTS: []}

    with open(full_path, READ_BINARY) as source_file:
        while True:
            block = source_file.read(MAX_BLOCK_SIZE)

            if not block:
                break

            url, _ = libernet.block.store(block, storage)
            description[CONTENTS].append({URL: url, SIZE: len(block)})

    return description


def __serialize_bundle(description):
    """compact serialize a bundle"""
    return json.dumps(description, sort_keys=True, separators=(",", ":")).encode(
        "utf-8"
    )


def __deserialize_bundle(block):
    """compact serialize a bundle"""
    return json.loads(block.decode("utf-8"))


def __file_entry(
    file_path: str, relative_path: str, previous: dict, storage
) -> (str, dict):
    """returns relative_path, file_entry"""
    prexisting = previous[FILES].get(relative_path, None) if previous else None
    prexisting_contents = prexisting[CONTENTS] if prexisting else None
    entry = __file_metadata_entry(file_path)
    size_matches = prexisting and prexisting[SIZE] == entry[SIZE]
    time_difference = prexisting[MODIFIED] if prexisting else 0 - entry[MODIFIED]
    unmodified = size_matches and abs(time_difference) < SAME_TIME_VARIANCE_IN_SECONDS
    new_contents = None if unmodified else __file_contents(file_path, storage)
    entry.update({CONTENTS: prexisting_contents} if unmodified else new_contents)
    return relative_path, entry


def __list_directory(path: str) -> (list, list):
    """get all the files and empty directories in a given directory"""
    file_list = []
    dir_list = []
    empty_dirs = []

    for root, dirs, files in os.walk(path):
        dir_list.extend(__path_relative_to(os.path.join(root, d), path) for d in dirs)
        file_list.extend(__path_relative_to(os.path.join(root, f), path) for f in files)

    for directory in dir_list:
        if not any((f.startswith(directory + "/") for f in file_list)):
            empty_dirs.append(directory)

    return file_list, empty_dirs


def __create_raw_bundle(source_path: str, storage, previous: dict) -> dict:
    """Given a path to a directory, create a full, raw bundle"""
    description = {FILES: {}}
    file_list, empty_dirs = __list_directory(source_path)

    for file in file_list:
        file_path = os.path.join(source_path, file)
        description[FILES].update(
            dict([__file_entry(file_path, file, previous, storage)])
        )

    description[DIRECTORIES] = {
        d: os.readlink(os.path.join(source_path, d))
        if os.path.islink(os.path.join(source_path, d))
        else None
        for d in empty_dirs
    }

    if not description[DIRECTORIES]:
        del description[DIRECTORIES]

    return description


def __files_for_bundle(overhead: int, size_per_block: float, files: dict) -> list:
    """Get subset of files for the bundle to prevent being too large"""
    filenames = sorted(files, key=lambda f: len(files[f][CONTENTS]), reverse=True)
    bundle_blocks = 0
    bundle_names = []

    for name in filenames:  # most blocks to least blocks
        additional_blocks = len(files[name][CONTENTS])
        blocks_size = (bundle_blocks + additional_blocks) * size_per_block
        new_size = blocks_size + overhead

        if new_size >= MAX_BUNDLE_SIZE:
            break

        bundle_blocks += additional_blocks
        bundle_names.append(name)

    return bundle_names


def __thin_bundle(raw: dict, storage, encrypt) -> str:
    """for bundles that are too big, create sub-bundles"""
    raw[BUNDLES] = []
    bundle = __serialize_bundle(raw)
    files_size = len(__serialize_bundle(raw[FILES]))
    size_per_block = files_size / sum(
        len(raw[FILES][f].get(CONTENTS, [])) for f in raw[FILES]
    )
    subbundle_count = ((len(bundle) - 1) // MAX_BUNDLE_SIZE + 1) - 1
    overhead = len(bundle) - files_size + int(subbundle_count * size_per_block)
    files = raw[FILES]
    bundle_filenames = __files_for_bundle(overhead, size_per_block, files)
    raw[FILES] = {f: files[f] for f in bundle_filenames}
    bundle = __serialize_bundle(raw)
    assert len(bundle) <= MAX_BUNDLE_SIZE, len(bundle)

    for file in bundle_filenames:
        del files[file]

    while len(files) > 0:
        subraw = {FILES: {}}
        overhead = len(__serialize_bundle(subraw))
        bundle_filenames = __files_for_bundle(overhead, size_per_block, files)
        subraw[FILES] = {f: files[f] for f in bundle_filenames}
        subbundle = __serialize_bundle(subraw)
        assert len(subbundle) <= MAX_BUNDLE_SIZE, len(subbundle)
        url, _ = libernet.block.store(subbundle, storage)
        raw[BUNDLES].append(url)
        bundle = __serialize_bundle(raw)
        assert len(bundle) <= MAX_BUNDLE_SIZE, len(bundle)

        for file in bundle_filenames:
            del files[file]

    url, _ = libernet.block.store(bundle, storage, encrypt)
    return url


def create(path: str, storage, previous: dict = None, encrypt=True, **kwargs) -> str:
    """stores a bundle from path in storage and returns the url
    storage - an object that can be called with put((block_url, block_data))
    previous - a bundle dictionary for optimization, see inflate()
    kwargs - added to the bundle description
    """
    # TODO: support providing mime types  # pylint: disable=fixme
    raw = __create_raw_bundle(path, storage, previous)
    raw.update(kwargs)
    bundle = __serialize_bundle(raw)

    if len(bundle) > MAX_BUNDLE_SIZE:
        return __thin_bundle(raw, storage, encrypt)

    url, _ = libernet.block.store(bundle, storage, encrypt)
    return url


def inflate(url: str, storage) -> dict:
    """inflates a bundle from the given url
    storage - a dict-like object
    returns as much as could be inflated
    """
    bundle_data = libernet.block.fetch(url, storage)

    if bundle_data is None:
        return None

    bundle = __deserialize_bundle(bundle_data)
    missing = []

    for suburl in bundle.get(BUNDLES, []):
        bundle_data = libernet.block.fetch(suburl, storage)

        if bundle_data is None:
            missing.append(suburl)
        else:
            subbundle = __deserialize_bundle(bundle_data)
            bundle[FILES].update(subbundle[FILES])

    if missing:
        bundle[BUNDLES] = missing
    elif BUNDLES in bundle:
        del bundle[BUNDLES]

    return bundle


def __find_missing_blocks(bundle: dict, target_dir: str, storage) -> (list, dict):
    """returns missing blocks and existing file metadata cache
    valid[file] == None => existing file is good
    valid[file] == {metadata} => file was modified (should not exist)
    """
    missing = [libernet.block.address(u) for u in bundle.get(BUNDLES, [])]
    valid = {}

    for file in bundle[FILES]:
        local_path = os.path.join(target_dir, file)
        entry = bundle[FILES][file]
        prexisting = None

        if os.path.isfile(local_path):
            prexisting = __file_metadata_entry(local_path)

        size_matches = prexisting and prexisting[SIZE] == entry[SIZE]
        time_difference = (prexisting[MODIFIED] if prexisting else 0) - entry[MODIFIED]
        unmodified = (
            size_matches and abs(time_difference) < SAME_TIME_VARIANCE_IN_SECONDS
        )

        if unmodified:
            valid[file] = True
        else:
            urls = [
                libernet.block.address(b["url"]) for b in bundle[FILES][file][CONTENTS]
            ]
            missing.extend(u for u in urls if u not in storage)

            if prexisting:
                valid[file] = False

    return missing, valid


def __remove_not_in_bundle(bundle: dict, target_dir: str):
    """Remove files and directories in target_dir that are not in bundle"""
    file_list, _ = __list_directory(target_dir) if target_dir else ([], [])

    for file in file_list:
        if file not in bundle[FILES]:
            os.remove(os.path.join(target_dir, file))

    _, dir_list = __list_directory(target_dir) if target_dir else ([], [])
    dir_list.sort(reverse=True)  # longer paths first

    for directory in dir_list:
        if directory not in bundle.get(DIRECTORIES, []):
            os.rmdir(os.path.join(target_dir, directory))


def __remove_modified_files(files_valid: dict, target_dir: str):
    """remove files that have been modified in target_dir since bundle was created"""
    modified = [f for f in files_valid if not files_valid[f]]

    for file in modified:
        os.remove(os.path.join(target_dir, file))


def __restore_file(bundle: dict, file: str, target_dir: str, storage):
    """restore a given file from a bundle to disk"""
    entry = bundle[FILES][file]
    file_path = os.path.join(target_dir, file)
    is_readonly = entry.get(READONLY, False)
    is_executable = entry.get(EXUTABLE, False)
    os.makedirs(os.path.split(file_path)[0], exist_ok=True)
    link_contents = entry.get("link", None)

    if link_contents is not None:
        os.symlink(link_contents, file_path)
        return

    with open(file_path, "wb") as file_contents:
        for block_info in entry[CONTENTS]:
            data = libernet.block.fetch(block_info["url"], storage)
            file_contents.write(data)

    # TODO: add xattr  # pylint: disable=fixme
    # TODO: add rsrc  # pylint: disable=fixme

    __set_mod_time(file_path, entry[MODIFIED])

    if is_readonly or is_executable:
        mode = os.stat(file_path).st_mode
        mode = (mode & ~stat.S_IWUSR) if is_readonly else mode
        mode = mode | (stat.S_IXUSR if is_executable else 0)
        os.chmod(file_path, mode)


def restore(url_or_bundle, target_dir: str, storage) -> list:
    """restores a bundle to a target directory if all data is available
    If not all blocks are available to restore, nothing is done
        and a list of (some) missing blocks is returned
    url_or_bundle - may be a url string from create() or bundle from inflate()
    target_dir - will be created if it doesn't exist
                contents will be updated to match the bundle
    storage - a dict-like object
    returns a list of missing blocks (may not be exhaustive) or None
    """
    # TODO: support URLs that have the path in the bundle  # pylint: disable=fixme
    bundle = (
        inflate(url_or_bundle, storage)
        if isinstance(url_or_bundle, str)
        else url_or_bundle
    )

    if bundle is None:  # NOT TESTED
        return [libernet.block.address(url_or_bundle)]

    missing, files_valid = __find_missing_blocks(bundle, target_dir, storage)

    if missing:  # there are blocks missing so do not restore
        return missing  # NOT TESTED

    __remove_not_in_bundle(bundle, target_dir)
    __remove_modified_files(files_valid, target_dir)
    files_to_restore = [f for f in bundle[FILES] if not files_valid.get(f, False)]

    for file in files_to_restore:
        __restore_file(bundle, file, target_dir, storage)

    for directory in bundle.get(DIRECTORIES, []):
        directory_path = os.path.join(target_dir, directory)

        if bundle[DIRECTORIES][directory] is None:
            os.makedirs(directory_path, exist_ok=True)
            continue

        os.symlink(bundle[DIRECTORIES][directory], directory_path)

    return None
