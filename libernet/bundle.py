#!/usr/bin/env python3

""" Libernet bundle creator
"""

import argparse
import os
import zlib

import libernet.plat.dirs
import libernet.tools.hash
import libernet.tools.encrypt


BLOCK_SIZE = 1024 * 1024


def store_block(contents, storage):
    """ Stores a block of data (no more than 1 MiB in size)"""
    assert len(contents) <= BLOCK_SIZE, f"Block too big {len(contents)}"
    compressed = zlib.compress(contents, 9)
    contents_identifier = libernet.tools.hash.sha256_data_identifier(contents)

    if len(contents) < len(compressed):
        compressed = contents

    key = libernet.tools.hash.binary_from_identifier(contents_identifier)
    encrypted = libernet.tools.encrypt.aes_encrypt(key, compressed)
    identifier = libernet.tools.hash.sha256_data_identifier(encrypted)
    upload_dir = os.path.join(storage, "upload", "local")
    sha_dir = os.path.join(upload_dir, "sha256", identifier[:2])
    data_path = os.path.join(sha_dir, identifier + '.raw')
    aes_dir = os.path.join(sha_dir, identifier, 'aes256')
    contents_path = os.path.join(aes_dir, contents_hash + '.raw')
    make_dirs(aes_dir)

    with open(data_path, 'wb') as data_file:
        data_file.write(encrypted)

    with open(contents_path, 'wb') as contents_file:
        contents_file.write(contents)

    url = f"/sha256/{identifier}/aes256/{contents_hash}"
    return url


def retrieve_block(url, storage):
    url_parts = url.split('/')
    # ['', 'sha256', '[identifier]']
    # ['', 'sha256', '[identifier]', 'aes256', '[key]']
    decrypt_url = len(url_parts) == 5
    assert len(url_parts) >= 3, f"URL too short {url}"
    assert decrypt_url or len(url_parts) == 3, f"incorrect url format: {url}"
    assert url_parts[0] == '', f"URL not absolutet {url}"
    assert url_parts[1] == 'sha256', f"URL not sha256 {url}"
    assert len(url_parts) <= 3 or url_parts[3] == 'aes256' and decrypt_url, f"URL not aes256 {url}"
    upload_dir = os.path.join(storage, 'upload')
    search_dirs = [os.path.join(storage, 'web')]
    search_dirs = [os.path.join(upload_dir, d)
                    for d in os.listdir(upload_dir)
                        if os.path.isdir(os.path.join(upload_dir, d))]

    search_dirs.insert(0, os.path.join(storage, 'web'))

    if upload_dir in search_dirs:
        search_dirs.remove(upload_dir)
        search_dirs.insert(0, os.path.join(upload_dir, 'local'))

    for search_dir in search_dirs:
        encrypted_path = os.path.join(search_dir, 'sha256', url_parts[2][:2], url_parts[2])

        if not os.path.isfile(encrypted_path + '.raw'):
            continue  # encrypted block not found in this dir, go to next

        if decrypt_url:
            full_path = os.path.join(encrypted_path, 'aes256', url_parts[4])

            if os.path.isfile(full_path + '.raw'):
                with open(full_path + '.raw', 'rb') as data_file:
                    return data_file.read()

            else:
                with open(encrypted_path + '.raw', 'rb') as encrypted_file:
                    encrypted = encrypted_file.read()

                key = aes_key_from_identifier(url_parts[4])
                compressed = libernet.tools.encrypt.aes_decrypt(key, encrypted_data)
                compressed_identifier = libernet.tools.hash.sha256_data_identifier(compressed)
                contents = None

                if compressed_identifier == url_parts[4]:
                    contents = compressed
                else:
                    contents = zlib.decompress(compressed)
                    contents_identifier = libernet.tools.hash.sha256_data_identifier(contents)

                    if contents_identifier != url_parts[4]:
                        contents = None

                if contents is not None:
                    with open(full_path + '.raw', 'wb') as data_file:
                        data_file.write(contents)

                    return contents

        else:
            if os.path.isfile(encrypted_path + '.raw'):
                with open(encrypted_path + '.raw', 'rb') as encrypted_file:
                    encrypted_data = encrypted_file.read()

                    if data_identifier(encrypted_data) == url_parts[2]:
                        return encrypted_data

    return None


def parse_args():
    """Parses and returns command line arguments."""
    parser = argparse.ArgumentParser(description="Libernet bundle creator")
    parser.add_argument(
        "action",
        choices=["store", "restore"],
        help="Action to perform",
    )
    parser.add_argument(
        "-s",
        "--storage",
        default=libernet.plat.dirs.pref_dir("libernet"),
        help="Directory to store/retrieve data",
    )
    parser.add_argument(
        "-d", "--dir", required=True, type=str, help="Directory to create a bundle for"
    )
    args = parser.parse_args()
    return args


def main():
    """Entry point. Loop forever unless we are told not to."""
    args = parse_args()
    print(args)


if __name__ == "__main__":
    main()
