#!/usr/bin/env python3

import tempfile
import os

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
def test_store_retrieve_block():
    with tempfile.TemporaryDirectory() as storage:
        libernet.plat.dirs.make_dirs(os.path.join(storage, "upload", "local"))
        urls = [libernet.tools.blocks.store_block(c, storage) for c in CONTENTS]
        block_urls = [u.rsplit('/', 2)[0] for u in urls]
        contents = [libernet.tools.blocks.retrieve_block(u, storage) for u in urls]

        for content, CONTENT in zip(contents,CONTENTS):
            assert content == CONTENT, f"'{content}' != '{CONTENT}'"

        for root, dirs, files in os.walk(storage):
            if os.path.split(root)[1] == "aes256":
                for file in files:
                    if os.path.splitext(file)[1] == '.raw':
                        os.remove(os.path.join(root, file))

        contents = [libernet.tools.blocks.retrieve_block(u, storage) for u in urls]
        block_contents = [libernet.tools.blocks.retrieve_block(u, storage) for u in block_urls]

        for content, CONTENT in zip(contents,CONTENTS):
            assert content == CONTENT, f"'{content}' != '{CONTENT}'"

        for root, dirs, files in os.walk(storage):
            if os.path.split(os.path.split(root)[0])[1] == "sha256":
                for file in files:
                    if os.path.splitext(file)[1] == '.raw':
                        os.remove(os.path.join(root, file))

        contents = [libernet.tools.blocks.retrieve_block(u, storage) for u in urls]
        block_contents = [libernet.tools.blocks.retrieve_block(u, storage) for u in block_urls]
        assert all([c is None for c in contents])
        assert all([c is None for c in block_contents])

        for root, dirs, files in os.walk(storage):
            if os.path.split(root)[1] == "aes256":
                for file in files:
                    if os.path.splitext(file)[1] == '.raw':
                        with open(os.path.join(root, file), 'wb') as data_file:
                            data_file.seek(0)
                            data_file.write(b'00000')

        contents = [libernet.tools.blocks.retrieve_block(u, storage) for u in urls]
        block_contents = [libernet.tools.blocks.retrieve_block(u, storage) for u in block_urls]
        assert all([c is None for c in contents])
        assert all([c is None for c in block_contents])

        for root, dirs, files in os.walk(storage):
            if os.path.split(os.path.split(root)[0])[1] == "sha256":
                for file in files:
                    if os.path.splitext(file)[1] == '.raw':
                        with open(os.path.join(root, file), 'wb') as data_file:
                            data_file.seek(0)
                            data_file.write(b'00000')

        contents = [libernet.tools.blocks.retrieve_block(u, storage) for u in urls]
        block_contents = [libernet.tools.blocks.retrieve_block(u, storage) for u in block_urls]
        assert all([c is None for c in contents])
        assert all([c is None for c in block_contents])

