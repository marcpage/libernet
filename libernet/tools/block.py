#!/usr/bin/env python3

""" Libernet block management
"""

import os
import zlib

import libernet.plat.dirs
import libernet.tools.hash
import libernet.tools.encrypt


BLOCK_SIZE = 1024 * 1024


def store_block(contents, storage):
    """Stores a block of data (no more than 1 MiB in size)"""
    assert (
        len(contents) <= BLOCK_SIZE
    ), f"Block too big {len(contents)} vs {BLOCK_SIZE} ({len(contents) - BLOCK_SIZE} bytes too big)"
    compressed = zlib.compress(contents, 9)
    contents_identifier = libernet.tools.hash.sha256_data_identifier(contents)

    if len(contents) < len(compressed):
        compressed = contents

    key = libernet.tools.hash.binary_from_identifier(contents_identifier)
    encrypted = libernet.tools.encrypt.aes_encrypt(key, compressed)
    identifier = libernet.tools.hash.sha256_data_identifier(encrypted)
    upload_dir = os.path.join(storage, "upload", "local")
    sha_dir = os.path.join(upload_dir, "sha256", identifier[:2])
    data_path = os.path.join(sha_dir, identifier + ".raw")
    aes_dir = os.path.join(sha_dir, identifier, "aes256")
    contents_path = os.path.join(aes_dir, contents_identifier + ".raw")
    libernet.plat.dirs.make_dirs(aes_dir)

    with open(data_path, "wb") as data_file:
        data_file.write(encrypted)

    with open(contents_path, "wb") as contents_file:
        contents_file.write(contents)

    url = f"/sha256/{identifier}/aes256/{contents_identifier}"
    return url


def validate_url(url):
    """Verify the URL matches a format we can extract"""
    url_parts = url.split("/")
    # ['', 'sha256', '[identifier]']
    # ['', 'sha256', '[identifier]', 'aes256', '[key]']
    decrypt_url = len(url_parts) == 5
    assert len(url_parts) >= 3, f"URL too short {url}"
    assert decrypt_url or len(url_parts) == 3, f"incorrect url format: {url}"
    assert url_parts[0] == "", f"URL not absolutet {url}"
    assert url_parts[1] == "sha256", f"URL not sha256 {url}"
    assert (
        len(url_parts) <= 3 or url_parts[3] == "aes256" and decrypt_url
    ), f"URL not aes256 {url}"
    return (url_parts[2], url_parts[4] if decrypt_url else None)


def find_block(search_dir, block_identifier, block_key):
    """Find a block in a directory"""
    encrypted_path = os.path.join(
        search_dir, "sha256", block_identifier[:2], block_identifier
    )
    if not os.path.isfile(encrypted_path + ".raw"):
        return None

    if block_key is not None:
        full_path = os.path.join(encrypted_path, "aes256", block_key)

        if os.path.isfile(full_path + ".raw"):
            with open(full_path + ".raw", "rb") as data_file:
                return data_file.read()

        else:
            with open(encrypted_path + ".raw", "rb") as encrypted_file:
                encrypted = encrypted_file.read()

            key = libernet.tools.hash.binary_from_identifier(block_key)
            compressed = libernet.tools.encrypt.aes_decrypt(key, encrypted)
            compressed_identifier = libernet.tools.hash.sha256_data_identifier(
                compressed
            )
            contents = None

            if compressed_identifier == block_key:
                contents = compressed
            else:
                contents = zlib.decompress(compressed)
                contents_identifier = libernet.tools.hash.sha256_data_identifier(
                    contents
                )

                if contents_identifier != block_key:
                    contents = None

            if contents is not None:
                with open(full_path + ".raw", "wb") as data_file:
                    data_file.write(contents)

                return contents

    else:
        with open(encrypted_path + ".raw", "rb") as encrypted_file:
            encrypted_data = encrypted_file.read()

        calculated_identifier = libernet.tools.hash.sha256_data_identifier(
            encrypted_data
        )

        if calculated_identifier == block_identifier:
            return encrypted_data

    return None


def retrieve_block(url, storage):
    """retrieve a block of data (optionally decrypting it)"""
    block_identifier, block_key = validate_url(url)
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

    for search_dir in search_dirs:
        found = find_block(search_dir, block_identifier, block_key)

        if found is not None:
            return found

    return None
