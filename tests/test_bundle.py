#!/usr/bin/env python3

import tempfile
import os
import multiprocessing
import time

import libernet.tools.bundle
import libernet.tools.block

def test_create():
    with tempfile.TemporaryDirectory() as storage:
        urls = libernet.tools.bundle.create("libernet", storage)
        more_urls = libernet.tools.bundle.create("libernet", storage, urls[0])

def test_create_large_files():
    libernet.tools.block.BLOCK_SIZE = 128 * 1024  # hack the block size to speed up the tests
    file_size = 16 * 1024 * 1024
    file_count = 32
    with tempfile.TemporaryDirectory() as storage:
        with tempfile.TemporaryDirectory() as to_store:
            for file_index in range(0, file_count):
                with open(os.path.join(to_store, f"{file_index:x}.txt"), "w") as text_file:
                    text_file.write('0'*file_size)

            start = time.time()
            urls = libernet.tools.bundle.create(to_store, storage, max_threads=multiprocessing.cpu_count())
            mid = time.time()
            more_urls = libernet.tools.bundle.create(to_store, storage, urls[0], max_threads=multiprocessing.cpu_count())
            done = time.time()
            assert (done - mid) < (mid - start) / 2
            assert not libernet.tools.bundle.missing_blocks(urls[0], storage)
            assert len(libernet.tools.bundle.load_raw(urls[0], storage).get('bundles', [])) > 0

            with tempfile.TemporaryDirectory() as to_restore:
                libernet.tools.bundle.restore(urls[0], to_restore, storage)
