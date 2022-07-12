#!/usr/bin/env python3

import tempfile
import os
import multiprocessing
import time

import libernet.tools.bundle

def test_create():
    with tempfile.TemporaryDirectory() as storage:
        urls = libernet.tools.bundle.create("libernet", storage)
        more_urls = libernet.tools.bundle.create("libernet", storage, urls[0])

def test_create_large_files():
    file_size = 2 * 1024 * 1024 * 1024
    with tempfile.TemporaryDirectory() as storage:
        with tempfile.TemporaryDirectory() as to_store:
            with open(os.path.join(to_store, "a.txt"), "w") as text_file:
                text_file.write('a'*file_size)

            with open(os.path.join(to_store, "b.txt"), "w") as text_file:
                text_file.write('b'*file_size)

            with open(os.path.join(to_store, "c.txt"), "w") as text_file:
                text_file.write('c'*file_size)

            with open(os.path.join(to_store, "d.txt"), "w") as text_file:
                text_file.write('d'*file_size)

            with open(os.path.join(to_store, "e.txt"), "w") as text_file:
                text_file.write('e'*file_size)

            with open(os.path.join(to_store, "f.txt"), "w") as text_file:
                text_file.write('f'*file_size)

            with open(os.path.join(to_store, "g.txt"), "w") as text_file:
                text_file.write('g'*file_size)

            with open(os.path.join(to_store, "h.txt"), "w") as text_file:
                text_file.write('h'*file_size)

            start = time.time()
            urls = libernet.tools.bundle.create(to_store, storage, max_threads=multiprocessing.cpu_count())
            mid = time.time()
            more_urls = libernet.tools.bundle.create(to_store, storage, urls[0], max_threads=multiprocessing.cpu_count())
            done = time.time()
            assert (done - mid) < (mid - start) / 2
