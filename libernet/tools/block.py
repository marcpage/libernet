#!/usr/bin/env python3

""" Libernet block management
"""

import os
import zlib

import libernet.plat.dirs
import libernet.tools.hash
import libernet.tools.encrypt


BLOCK_SIZE = 1024 * 1024
BLOCK_TOP_DIR_SIZE = 3  # number of characters in block grouping directory name
MINIMUM_MATCH_FOR_LIKE = 4  # 4 is about 1 seconds, 5 is about 10 seconds to generate


def block_dir(storage_part, identifier, key=None, full=False):
    """Find the path to various block directories"""
    directory = os.path.join(storage_part, "sha256", identifier[:BLOCK_TOP_DIR_SIZE])

    if not full and key is None:
        return directory

    if key is None:
        return os.path.join(directory, identifier)

    aes_dir = os.path.join(directory, identifier, "aes256")
    libernet.plat.dirs.make_dirs(aes_dir)

    if not full:
        return aes_dir

    return os.path.join(aes_dir, key)


def store_block(contents, storage, encrypt=True):
    """ Stores a block of data (no more than 1 MiB in size) 
        returns the url for the block
    """
    assert (
        len(contents) <= BLOCK_SIZE
    ), f"Block too big {len(contents)} vs {BLOCK_SIZE} ({len(contents) - BLOCK_SIZE} bytes too big)"
    compressed = zlib.compress(contents, 9)
    contents_identifier = libernet.tools.hash.sha256_data_identifier(contents)

    if len(contents) < len(compressed):
        compressed = contents

    if encrypt:
        key = libernet.tools.hash.binary_from_identifier(contents_identifier)
        block_contents = libernet.tools.encrypt.aes_encrypt(key, compressed)
        identifier = libernet.tools.hash.sha256_data_identifier(block_contents)
    else:
        identifier = contents_identifier
        block_contents = compressed

    upload_dir = os.path.join(storage, "upload", "local")
    data_path = block_dir(upload_dir, identifier, full=True) + ".raw"

    if encrypt:
        contents_path = (
            block_dir(upload_dir, identifier, contents_identifier, full=True) + ".raw"
        )

    else:
        libernet.plat.dirs.make_dirs(
            block_dir(upload_dir, identifier, contents_identifier)
        )

    with open(data_path, "wb") as data_file:
        data_file.write(block_contents)

    if encrypt:
        with open(contents_path, "wb") as contents_file:
            contents_file.write(contents)

        return f"/sha256/{identifier}/aes256/{contents_identifier}"

    return f"/sha256/{identifier}"


def validate_url(url):
    """Verify the URL matches a format we can extract"""
    url_parts = url.split("/")
    # ['', 'sha256', '[identifier]']
    # ['', 'sha256', '[identifier]', 'aes256', '[key]']
    decrypt_url = len(url_parts) >= 5
    bundle_path = len(url_parts) > 5
    assert len(url_parts) >= 3, f"URL too short {url}"
    assert decrypt_url or len(url_parts) == 3, f"incorrect url format: {url}"
    assert url_parts[0] == "", f"URL not absolutet {url}"
    assert url_parts[1] == "sha256", f"URL not sha256 {url}"
    assert not decrypt_url or url_parts[3] == "aes256", f"URL not aes256 {url}"
    path = "/".join(url_parts[5:]) if bundle_path else None
    return (url_parts[2], url_parts[4] if decrypt_url else None, path)


def decrypt_block(encrypted_path, block_key):
    """decrypt a block"""
    full_path = os.path.join(encrypted_path, "aes256", block_key)

    if os.path.isfile(full_path + ".raw"):

        with open(full_path + ".raw", "rb") as data_file:
            return data_file.read()

    else:
        with open(encrypted_path + ".raw", "rb") as encrypted_file:
            encrypted = encrypted_file.read()
        key = libernet.tools.hash.binary_from_identifier(block_key)
        compressed = libernet.tools.encrypt.aes_decrypt(key, encrypted)
        compressed_identifier = libernet.tools.hash.sha256_data_identifier(compressed)
        contents = None

        if compressed_identifier == block_key:
            contents = compressed
        else:
            try:
                contents = zlib.decompress(compressed)
                contents_identifier = libernet.tools.hash.sha256_data_identifier(
                    contents
                )

            except zlib.error:
                contents_identifier = None

            if contents_identifier != block_key:
                contents = None

        if contents is not None:
            libernet.plat.dirs.make_dirs(os.path.split(full_path)[0])
            with open(full_path + ".raw", "wb") as data_file:
                data_file.write(contents)

            return contents

    return None


def find_block(search_dir, block_identifier, block_key=None, load=True):
    """ Find a block in a directory 
        returns
            None - if the .raw file does not exist in the directory
            None - if the identifier is not found
            True - if the .raw file exists and load is False
            contents - if the contents can be retrieved

    """
    encrypted_path = block_dir(search_dir, block_identifier, full=True)
    if not os.path.isfile(encrypted_path + ".raw"):
        return None

    if not load:
        return True

    if block_key is not None:
        contents = decrypt_block(encrypted_path, block_key)

        if contents is not None:
            return contents

    else:
        with open(encrypted_path + ".raw", "rb") as encrypted_file:
            encrypted_data = encrypted_file.read()

        calculated_identifier = libernet.tools.hash.sha256_data_identifier(
            encrypted_data
        )
        if calculated_identifier == block_identifier:
            return encrypted_data

        uncompressed = zlib.decompress(encrypted_data)
        calculated_identifier = libernet.tools.hash.sha256_data_identifier(uncompressed)

        if calculated_identifier == block_identifier:
            return uncompressed

    return None


def get_search_dirs(storage):
    """get a list of directories to search for blocks in the storage"""
    upload_dir = os.path.join(storage, "upload")
    upload_local_dir = os.path.join(upload_dir, "local")
    search_dirs = [os.path.join(storage, "web")]
    search_dirs = [
        os.path.join(upload_dir, d)
        for d in os.listdir(upload_dir)
        if os.path.isdir(os.path.join(upload_dir, d))
    ]
    search_dirs.insert(0, os.path.join(storage, "web"))

    if upload_local_dir in search_dirs:
        search_dirs.remove(upload_local_dir)
        search_dirs.insert(0, upload_local_dir)

    return search_dirs


def get_contents(storage, block_identifier, block_key=None, load=True):
    """given identifiers, load a block (or check that the block exists) or return None"""
    search_dirs = get_search_dirs(storage)

    for search_dir in search_dirs:
        found = find_block(search_dir, block_identifier, block_key, load)

        if found is not None:
            return found

    return None


def retrieve(url, storage, load=True):
    """retrieve a block of data (optionally decrypting it)"""
    block_identifier, block_key, _ = validate_url(url)
    return get_contents(storage, block_identifier, block_key, load)


def like(url, storage):
    """Find identifiers 'like' the given one"""
    identifier, _, _ = validate_url(url)
    search_dirs = [block_dir(d, identifier) for d in get_search_dirs(storage)]
    found = []
    prefix = identifier[:MINIMUM_MATCH_FOR_LIKE]

    for search_dir in search_dirs:
        if os.path.isdir(search_dir):
            found.extend(
                [
                    os.path.splitext(i)[0]
                    for i in os.listdir(search_dir)
                    if i.endswith(".raw") and i.startswith(prefix)
                ]
            )

    return [f"/sha256/{i}" for i in found]
