#!/usr/bin/env python3

import tempfile
import os
import multiprocessing
import time
import shutil

import libernet.tools.bundle
import libernet.tools.block

def test_create():
    with tempfile.TemporaryDirectory() as storage:
        urls = libernet.tools.bundle.create("libernet", storage)
        more_urls = libernet.tools.bundle.create("libernet", storage, urls[0])

def test_create_large_files():
    libernet.tools.block.BLOCK_SIZE = 128 * 1024  # hack the block size to speed up the tests
    file_size = 16 * 1024 * 1024  # average file size in bundle is 22k
    file_count = 64
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
            assert (done - mid) < (mid - start) / 2, (f"2nd store ({done-mid:0.3f} seconds) should have " 
                                                        + f"been at least half 1st store ({mid-start:0.3f} seconds)")
            assert not libernet.tools.bundle.missing_blocks(urls[0], storage), "Blocks are missing but shouldn't be"
            assert len(libernet.tools.bundle.load_raw(urls[0], storage).get('bundles', [])) >= 2, "we should have at least 2 sub-bundles"

            with tempfile.TemporaryDirectory() as to_restore:
                bundle = libernet.tools.bundle.Path(urls[0], storage)
                assert not bundle.missing_blocks()
                bundle.restore_file(to_restore)

def test_non_existent_bundle():
    with tempfile.TemporaryDirectory() as storage:
        urls = libernet.tools.bundle.create("libernet", storage, verbose=True)
        bundle_url = urls[0]
        assert len(libernet.tools.bundle.missing_blocks(bundle_url, storage)) == 0
        local_storage = os.path.join(storage, 'upload', 'local')
        bundle = libernet.tools.bundle.Path(bundle_url, storage)
        total_count = len(urls) - 1
        assert libernet.tools.bundle.Path(bundle_url, storage).missing_blocks("does not exist") == None

        for index, url in enumerate(reversed(urls)):
            identifier, _, _ = libernet.tools.block.validate_url(url)
            block_path = libernet.tools.block.block_dir(local_storage, identifier, full=True)
            os.unlink(block_path + '.raw')
            shutil.rmtree(block_path)
            missing = libernet.tools.bundle.missing_blocks(bundle_url, storage)
            assert len(missing) == 1 or len(missing) == (index + 1)
            
            with tempfile.TemporaryDirectory() as to_restore:
                try:
                    bundle.restore_file(to_restore, "__init__.py")
                    assert os.path.isfile(os.path.join(to_restore, "__init__.py"))
                except FileNotFoundError:
                    pass

            with tempfile.TemporaryDirectory() as to_restore:
                try:
                    bundle.restore_file(to_restore)
                    assert False, "We should have thrown a file-not-found exception"
                except FileNotFoundError:
                    pass
            
            with tempfile.TemporaryDirectory() as to_restore:
                try:
                    found = libernet.tools.bundle.Path(bundle_url, storage).restore_file(to_restore)
                    assert found == None and index == total_count, "We should have thrown a file-not-found exception or had None"
                except FileNotFoundError:
                    pass

            path_missing = libernet.tools.bundle.Path(bundle_url + '/__init__.py', storage).missing_blocks()
            assert len(path_missing) == 0 or len(path_missing) == 1
            path_missing = libernet.tools.bundle.Path(bundle_url, storage).missing_blocks()
            assert index == total_count or len(path_missing) == index + 1
            assert index < total_count or len(path_missing) == 1
            files = libernet.tools.bundle.get_files(bundle_url, storage)
            assert index == total_count or len(files['files']) == total_count
            assert index != total_count or len(files['files']) == 0
            files = libernet.tools.bundle.get_files(bundle_url, storage, enforce=True)
            assert files is None or len(files['files']) == total_count
            assert index < total_count or files is None

test_non_existent_bundle()
