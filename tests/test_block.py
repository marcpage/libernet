#!/usr/bin/env python3

import tempfile
import os
import struct

import libernet.tools.block
import libernet.plat

CONTENTS = [
    b'test',
    b'',
    b'abcdefghijklmnopqrstuvwxyz',
    b'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz',
    b'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789',
    b'00000000000000000000000000000000000000000000000000000000000000000000000000000000000',
]
def test_store_retrieve():
    with tempfile.TemporaryDirectory() as storage:
        libernet.plat.dirs.make_dirs(os.path.join(storage, "upload", "local"))
        urls = [libernet.tools.block.store_block(c, storage) for c in CONTENTS]
        block_urls = [u.rsplit('/', 2)[0] for u in urls]
        contents = [libernet.tools.block.retrieve(u, storage) for u in urls]

        for content, CONTENT in zip(contents,CONTENTS):
            assert content == CONTENT, f"'{content}' != '{CONTENT}'"

        for root, dirs, files in os.walk(storage):
            if os.path.split(root)[1] == "aes256":
                for file in files:
                    if os.path.splitext(file)[1] == '.raw':
                        os.remove(os.path.join(root, file))

        contents = [libernet.tools.block.retrieve(u, storage) for u in urls]
        block_contents = [libernet.tools.block.retrieve(u, storage) for u in block_urls]

        for content, CONTENT in zip(contents,CONTENTS):
            assert content == CONTENT, f"'{content}' != '{CONTENT}'"

        for root, dirs, files in os.walk(storage):
            if os.path.split(os.path.split(root)[0])[1] == "sha256":
                for file in files:
                    if os.path.splitext(file)[1] == '.raw':
                        os.remove(os.path.join(root, file))

        contents = [libernet.tools.block.retrieve(u, storage) for u in urls]
        block_contents = [libernet.tools.block.retrieve(u, storage) for u in block_urls]
        assert all([c is None for c in contents])
        assert all([c is None for c in block_contents])

        for root, dirs, files in os.walk(storage):
            if os.path.split(root)[1] == "aes256":
                for file in files:
                    if os.path.splitext(file)[1] == '.raw':
                        with open(os.path.join(root, file), 'wb') as data_file:
                            data_file.seek(0)
                            data_file.write(b'00000')

        contents = [libernet.tools.block.retrieve(u, storage) for u in urls]
        block_contents = [libernet.tools.block.retrieve(u, storage) for u in block_urls]
        assert all([c is None for c in contents])
        assert all([c is None for c in block_contents])

        for root, dirs, files in os.walk(storage):
            if os.path.split(os.path.split(root)[0])[1] == "sha256":
                for file in files:
                    if os.path.splitext(file)[1] == '.raw':
                        with open(os.path.join(root, file), 'wb') as data_file:
                            data_file.seek(0)
                            data_file.write(b'00000')

        contents = [libernet.tools.block.retrieve(u, storage) for u in urls]
        block_contents = [libernet.tools.block.retrieve(u, storage) for u in block_urls]
        assert all([c is None for c in contents])
        assert all([c is None for c in block_contents])


def test_validate_url():
    identifier = '0d94f058c890f5cf76aa6384a6f8ddea30fa69a21c8ea9a978298303d4c4ca01'
    key = 'd7551a83e211fe7b094efdf371a34772fecc1ca7e3c6143eaac6971d9824b9c2'
    base_url = f"/sha256/{identifier}/aes256/{key}"
    assert libernet.tools.block.validate_url(base_url) == (identifier, key, None)
    assert libernet.tools.block.validate_url(base_url+'/') == (identifier, key, '')
    assert libernet.tools.block.validate_url(base_url+'/test') == (identifier, key, 'test')
    assert libernet.tools.block.validate_url(base_url+'/test/me') == (identifier, key, 'test/me')


def test_missing_block():
    identifier = '0000000000000000000000000000000000000000000000000000000000000000'
    with tempfile.TemporaryDirectory() as storage:
        libernet.plat.dirs.make_dirs(os.path.join(storage, "upload", "local"))
        assert libernet.tools.block.get_contents(storage, identifier) == None


def test_compressed_block():
    contents = (
          b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000')

    with tempfile.TemporaryDirectory() as storage:
        local_dir = os.path.join(storage, "upload", "local")
        libernet.plat.dirs.make_dirs(local_dir)
        url = libernet.tools.block.store_block(contents, storage)
        identifier, key, _ = libernet.tools.block.validate_url(url)
        assert os.path.isdir(local_dir)
        contents_path = libernet.tools.block.block_dir(local_dir, identifier, key, full=True) + '.raw'
        assert os.path.isfile(contents_path)
        os.unlink(contents_path)
        read = libernet.tools.block.retrieve(url, storage)
        assert read == contents


def test_bad_key():
    bad_key = '0000000000000000000000000000000000000000000000000000000000000000'
    contents = (
          b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000')

    with tempfile.TemporaryDirectory() as storage:
        local_dir = os.path.join(storage, "upload", "local")
        libernet.plat.dirs.make_dirs(local_dir)
        url = libernet.tools.block.store_block(contents, storage)
        identifier, key, _ = libernet.tools.block.validate_url(url)
        assert os.path.isdir(local_dir)
        contents_path = libernet.tools.block.block_dir(local_dir, identifier, key, full=True) + '.raw'
        assert os.path.isfile(contents_path)
        os.unlink(contents_path)
        read = libernet.tools.block.retrieve(f"/sha256/{identifier}/aes256/{bad_key}", storage)
        assert read == None


def test_like():
    with tempfile.TemporaryDirectory() as storage:
        url = libernet.tools.block.store_block(b"test data", storage)
        identifier, _, _ = libernet.tools.block.validate_url(url)
        expected_matches = {f"/sha256/{identifier}"}
        data_to_store = [i for i in range(0, 2000)]
        data_to_store.extend((28349, 106512))  # matches

        for i in data_to_store:
            random_data = struct.pack("q", i)
            random_url = libernet.tools.block.store_block(random_data, storage)
            data_identifier, _, _ = libernet.tools.block.validate_url(random_url)

            if identifier.startswith(data_identifier[:libernet.tools.block.MINIMUM_MATCH_FOR_LIKE]):
                expected_matches.add(f"/sha256/{data_identifier}")

        assert set(libernet.tools.block.like(f"/sha256/{identifier}", storage)) == expected_matches
        assert len(expected_matches) == 3


def test_compressed_unecnrypted_block():
    contents = (
          b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000'
        + b'0000000000000000000000000000000000000000000000000000000000000000')

    with tempfile.TemporaryDirectory() as storage:
        local_dir = os.path.join(storage, "upload", "local")
        libernet.plat.dirs.make_dirs(local_dir)
        url = libernet.tools.block.store_block(contents, storage, encrypt=False)
        identifier, _, _ = libernet.tools.block.validate_url(url)
        assert os.path.isdir(local_dir)
        contents_path = libernet.tools.block.block_dir(local_dir, identifier, full=True) + '.raw'
        assert os.path.isfile(contents_path)
        read = libernet.tools.block.retrieve(url, storage)
        assert read == contents


def test_uncompressed_unecnrypted_block():
    contents = (b'12345')

    with tempfile.TemporaryDirectory() as storage:
        local_dir = os.path.join(storage, "upload", "local")
        libernet.plat.dirs.make_dirs(local_dir)
        url = libernet.tools.block.store_block(contents, storage, encrypt=False)
        identifier, _, _ = libernet.tools.block.validate_url(url)
        assert os.path.isdir(local_dir)
        contents_path = libernet.tools.block.block_dir(local_dir, identifier, full=True) + '.raw'
        assert os.path.isfile(contents_path)
        read = libernet.tools.block.retrieve(url, storage)
        assert read == contents
