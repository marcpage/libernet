#!/usr/bin/env python3

import tempfile
import os
import multiprocessing
import time
import shutil
import stat

import libernet.tools.bundle
import libernet.tools.block
import libernet.plat.dirs
import libernet.plat.files

def test_create():
    with tempfile.TemporaryDirectory() as storage:
        urls = libernet.tools.bundle.create("libernet", storage, index=None, previous=None)
        assert len(set(urls)) == len(urls)
        more_urls = libernet.tools.bundle.create("libernet", storage, previous=urls[0])
        try:
            libernet.tools.bundle.Path(more_urls[0], storage).missing_blocks("")
            assert False, "We should have thrown an exception"
        
        except FileNotFoundError:
            pass


def test_create_large_files():
    threads = multiprocessing.cpu_count()
    old_block_size = libernet.tools.block.BLOCK_SIZE
    libernet.tools.block.BLOCK_SIZE = 128 * 1024  # hack the block size to speed up the tests
    libernet.tools.bundle.MAX_BUNDLE_SIZE = libernet.tools.block.BLOCK_SIZE  # hack bundle also
    file_size = 16 * 1024 * 1024  # average file size in bundle is 22k
    file_count = 64

    with tempfile.TemporaryDirectory() as storage:
        with tempfile.TemporaryDirectory() as to_store:
            for file_index in range(0, file_count):
                with open(os.path.join(to_store, f"{file_index:x}.txt"), "w") as text_file:
                    text_file.write('0'*file_size)

            start = time.time()
            urls = libernet.tools.bundle.create(to_store, storage, max_threads=threads)
            assert len(set(urls)) == len(urls)
            mid = time.time()
            more_urls = libernet.tools.bundle.create(to_store, storage, previous=urls[0], max_threads=threads)
            done = time.time()
            assert (done - mid) < (mid - start) / 2, (f"2nd store ({done-mid:0.3f} seconds) should have " 
                                                        + f"been at least half 1st store ({mid-start:0.3f} seconds)")
            assert not libernet.tools.bundle.missing_blocks(urls[0], storage), "Blocks are missing but shouldn't be"
            assert len(libernet.tools.bundle.load_raw(urls[0], storage).get('bundles', [])) >= 2, "we should have at least 2 sub-bundles"

            with tempfile.TemporaryDirectory() as to_restore:
                bundle = libernet.tools.bundle.Path(urls[0], storage)
                assert not bundle.missing_blocks()
                bundle.restore_file(to_restore)
    
    libernet.tools.block.BLOCK_SIZE = old_block_size
    libernet.tools.bundle.MAX_BUNDLE_SIZE = old_block_size


def test_non_existent_bundle():
    with tempfile.TemporaryDirectory() as storage:
        urls = libernet.tools.bundle.create("libernet", storage, verbose=True)
        assert len(set(urls)) == len(urls)
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


def test_empty_directories():
    with tempfile.TemporaryDirectory() as storage:
        with tempfile.TemporaryDirectory() as working:
            deep_dirs = [
                os.path.join("1", "2", "3"),
                os.path.join("1", "2", "4"),
                os.path.join("1", "5", "6"),
                os.path.join("7", "8"),
                os.path.join("9"),
                os.path.join("A", "B", "C"),
            ]

            for directory in deep_dirs:
                libernet.plat.dirs.make_dirs(os.path.join(working, directory))

            urls = libernet.tools.bundle.create(working, storage)
            assert len(set(urls)) == len(urls)

        with tempfile.TemporaryDirectory() as to_restore:
            bundle = libernet.tools.bundle.Path(urls[0], storage)
            assert not bundle.missing_blocks()
            bundle.restore_file(to_restore)
            
            for directory in deep_dirs:
                assert os.path.isdir(os.path.join(to_restore, directory)), f"Unable to restore {directory}"


def test_symlinks():
    data_contents = 'contents'

    with tempfile.TemporaryDirectory() as storage:
        with tempfile.TemporaryDirectory() as working:
            data_path = os.path.join(working, 'data.txt')
            inner_path = os.path.join(working, "inner")
            copy_path = os.path.join(working, "inner", "copy.txt")
            inner_link_path = os.path.join(working, "inner_link")
            inner_data_path = os.path.join(inner_link_path, 'copy.txt')

            with open(data_path, 'w') as text_file:
                text_file.write(data_contents)
            
            libernet.plat.dirs.make_dirs(inner_path)
            libernet.plat.files.symlink("../data.txt", copy_path)
            libernet.plat.files.symlink("inner", inner_link_path)

            with open(data_path, 'r') as text_file:
                assert text_file.read() == data_contents
            
            with open(copy_path, 'r') as text_file:
                assert text_file.read() == data_contents

            with open(inner_data_path, 'r') as text_file:
                assert text_file.read() == data_contents

            urls = libernet.tools.bundle.create(working, storage, index="data.txt")
            assert len(set(urls)) == len(urls)

        with tempfile.TemporaryDirectory() as to_restore:
            bundle = libernet.tools.bundle.Path(urls[0], storage)
            assert not bundle.missing_blocks()
            assert bundle.index() == "data.txt"
            bundle.restore_file(to_restore)
            data_path = os.path.join(to_restore, 'data.txt')
            inner_path = os.path.join(to_restore, "inner")
            copy_path = os.path.join(to_restore, "inner", "copy.txt")
            inner_link_path = os.path.join(to_restore, "inner_link")
            inner_data_path = os.path.join(inner_link_path, 'copy.txt')

            with open(data_path, 'r') as text_file:
                assert text_file.read() == data_contents
            
            with open(copy_path, 'r') as text_file:
                assert text_file.read() == data_contents

            with open(inner_data_path, 'r') as text_file:
                assert text_file.read() == data_contents


def test_permissions():
    data_contents = 'contents'

    with tempfile.TemporaryDirectory() as storage:
        with tempfile.TemporaryDirectory() as working:
            normal_path = os.path.join(working, 'normal.txt')
            readonly_path = os.path.join(working, 'readonly.txt')
            execute_path = os.path.join(working, 'execute.txt')
            readonly_execute_path = os.path.join(working, 'readonly_execute.txt')

            with open(normal_path, 'w') as text_file:
                text_file.write(data_contents)
            
            with open(readonly_path, 'w') as text_file:
                text_file.write(data_contents)
            
            with open(execute_path, 'w') as text_file:
                text_file.write(data_contents)
            
            with open(readonly_execute_path, 'w') as text_file:
                text_file.write(data_contents)
            
            os.chmod(readonly_path, 0o444)
            os.chmod(execute_path, 0o777)
            os.chmod(readonly_execute_path, 0o555)

            urls = libernet.tools.bundle.create(working, storage)
            assert len(set(urls)) == len(urls)

        with tempfile.TemporaryDirectory() as to_restore:
            bundle = libernet.tools.bundle.Path(urls[0], storage)
            assert not bundle.missing_blocks()
            bundle.restore_file(to_restore)
            normal_path = os.path.join(to_restore, 'normal.txt')
            readonly_path = os.path.join(to_restore, 'readonly.txt')
            execute_path = os.path.join(to_restore, 'execute.txt')
            readonly_execute_path = os.path.join(to_restore, 'readonly_execute.txt')
            assert os.stat(normal_path).st_mode & stat.S_IRUSR == stat.S_IRUSR
            assert os.stat(normal_path).st_mode & stat.S_IWUSR == stat.S_IWUSR
            assert os.stat(normal_path).st_mode & stat.S_IXUSR == 0
            assert os.stat(readonly_path).st_mode & stat.S_IRUSR == stat.S_IRUSR
            assert os.stat(readonly_path).st_mode & stat.S_IWUSR == 0
            assert os.stat(readonly_path).st_mode & stat.S_IXUSR == 0
            assert os.stat(execute_path).st_mode & stat.S_IRUSR == stat.S_IRUSR
            assert os.stat(execute_path).st_mode & stat.S_IWUSR == stat.S_IWUSR
            assert os.stat(execute_path).st_mode & stat.S_IXUSR == stat.S_IXUSR
            assert os.stat(readonly_execute_path).st_mode & stat.S_IRUSR == stat.S_IRUSR
            assert os.stat(readonly_execute_path).st_mode & stat.S_IWUSR == 0
            assert os.stat(readonly_execute_path).st_mode & stat.S_IXUSR == stat.S_IXUSR


def test_rsrc():
    data_contents = 'contents'
    rsrc_contents = 'rsrc'

    with tempfile.TemporaryDirectory() as storage:
        with tempfile.TemporaryDirectory() as working:
            normal_path = os.path.join(working, 'normal.txt')
            rsrc_path = os.path.join(normal_path, "..namedfork", "rsrc")

            with open(normal_path, 'w') as text_file:
                text_file.write(data_contents)
            
            with open(rsrc_path, 'w') as text_file:
                text_file.write(rsrc_contents)

            urls = libernet.tools.bundle.create(working, storage)
            assert len(set(urls)) == len(urls)

        with tempfile.TemporaryDirectory() as to_restore:
            bundle = libernet.tools.bundle.Path(urls[0], storage)
            assert not bundle.missing_blocks()
            bundle.restore_file(to_restore)
            normal_path = os.path.join(to_restore, 'normal.txt')
            rsrc_path = os.path.join(normal_path, "..namedfork", "rsrc")

            with open(normal_path, 'r') as text_file:
                assert text_file.read() == data_contents

            with open(rsrc_path, 'r') as text_file:
                assert text_file.read() == rsrc_contents


def test_missing_index():
    data_contents = 'contents'

    with tempfile.TemporaryDirectory() as storage:
        with tempfile.TemporaryDirectory() as working:
            normal_path = os.path.join(working, 'normal.txt')

            with open(normal_path, 'w') as text_file:
                text_file.write(data_contents)
            
            try:
                urls = libernet.tools.bundle.create(working, storage, index="index.html")
                assert False, "We should have thrown File not found"

            except FileNotFoundError:
                pass


def test_index():
    data_contents = 'contents'
    urls = None

    with tempfile.TemporaryDirectory() as storage:
        with tempfile.TemporaryDirectory() as working:
            normal_path = os.path.join(working, 'normal.txt')
            libernet.plat.dirs.make_dirs(os.path.join(working, "empty dir"))

            with open(normal_path, 'w') as text_file:
                text_file.write(data_contents)
            
            urls = libernet.tools.bundle.create(working, storage, index="normal.txt")
            assert len(set(urls)) == len(urls)
        
        bundle = libernet.tools.bundle.Path(urls[0], storage)
        assert bundle.index() == "normal.txt"
        identifier, key, _ = libernet.tools.block.validate_url(urls[0])
        assert bundle.relative_path("normal.txt") == f"sha256/{identifier[:libernet.tools.block.BLOCK_TOP_DIR_SIZE]}/{identifier}/aes256/{key}/normal.txt"
        assert len(bundle.missing_blocks("normal.txt")) == 0
        assert len(bundle.missing_blocks("empty dir")) == 0


def test_create_various_number_of_files():
    threads = multiprocessing.cpu_count()
    old_block_size = libernet.tools.block.BLOCK_SIZE
    libernet.tools.block.BLOCK_SIZE = 1 * 1024  # hack the block size to speed up the tests
    libernet.tools.bundle.MAX_BUNDLE_SIZE = libernet.tools.block.BLOCK_SIZE  # hack bundle also
    file_contents = "contents"
    
    for file_count in range(1, 22):
        with tempfile.TemporaryDirectory() as storage:
            with tempfile.TemporaryDirectory() as to_store:
                for file_index in range(0, file_count):
                    with open(os.path.join(to_store, f"{file_index:x}.txt"), "w") as text_file:
                        text_file.write(file_contents)

                urls = libernet.tools.bundle.create(to_store, storage, max_threads=threads)
                assert len(set(urls)) == len(urls)
                assert not libernet.tools.bundle.missing_blocks(urls[0], storage), "Blocks are missing but shouldn't be"

                with tempfile.TemporaryDirectory() as to_restore:
                    bundle = libernet.tools.bundle.Path(urls[0], storage)
                    assert not bundle.missing_blocks()
                    bundle.restore_file(to_restore)

                    for file_index in range(0, file_count):
                        assert len(bundle.missing_blocks(f"{file_index:x}.txt")) == 0


    libernet.tools.block.BLOCK_SIZE = old_block_size
    libernet.tools.bundle.MAX_BUNDLE_SIZE = old_block_size

def test_create_large_number_of_files():
    threads = multiprocessing.cpu_count()
    old_block_size = libernet.tools.block.BLOCK_SIZE
    libernet.tools.block.BLOCK_SIZE = 1 * 1024  # hack the block size to speed up the tests
    libernet.tools.bundle.MAX_BUNDLE_SIZE = libernet.tools.block.BLOCK_SIZE  # hack bundle also
    file_contents = "contents"
    file_count = 21
    
    with tempfile.TemporaryDirectory() as storage:
        storage = "/tmp/testing_bundle_stuff"
        if os.path.isdir(storage): shutil.rmtree(storage)
        libernet.plat.dirs.make_dirs(storage)
        with tempfile.TemporaryDirectory() as to_store:
            for file_index in range(0, file_count):
                with open(os.path.join(to_store, f"{file_index:x}.txt"), "w") as text_file:
                    text_file.write(file_contents)

            urls = libernet.tools.bundle.create(to_store, storage, max_threads=threads)
            assert len(set(urls)) == len(urls)

        assert not libernet.tools.bundle.missing_blocks(urls[0], storage), "Blocks are missing but shouldn't be"
        bundle = libernet.tools.bundle.Path(urls[0], storage)

        for url in urls[1:]:
            identifier, _, _ = libernet.tools.block.validate_url(url)
            block_path = os.path.join(storage, "upload", "local", "sha256", identifier[:libernet.tools.block.BLOCK_TOP_DIR_SIZE], identifier)
            shutil.rmtree(block_path)
            os.unlink(block_path + ".raw")
            assert sum([len(bundle.missing_blocks(f"{i:x}.txt")) for i in range(0, file_count)]) > 0

    libernet.tools.block.BLOCK_SIZE = old_block_size
    libernet.tools.bundle.MAX_BUNDLE_SIZE = old_block_size


def test_create_large_number_of_files_reverse():
    threads = multiprocessing.cpu_count()
    old_block_size = libernet.tools.block.BLOCK_SIZE
    libernet.tools.block.BLOCK_SIZE = 1 * 1024  # hack the block size to speed up the tests
    libernet.tools.bundle.MAX_BUNDLE_SIZE = libernet.tools.block.BLOCK_SIZE  # hack bundle also
    file_contents = "contents"
    file_count = 21
    
    with tempfile.TemporaryDirectory() as storage:
        storage = "/tmp/testing_bundle_stuff"
        if os.path.isdir(storage): shutil.rmtree(storage)
        libernet.plat.dirs.make_dirs(storage)
        with tempfile.TemporaryDirectory() as to_store:
            for file_index in range(0, file_count):
                with open(os.path.join(to_store, f"{file_index:x}.txt"), "w") as text_file:
                    text_file.write(file_contents)

            urls = libernet.tools.bundle.create(to_store, storage, max_threads=threads)
            assert len(set(urls)) == len(urls)

        assert not libernet.tools.bundle.missing_blocks(urls[0], storage), "Blocks are missing but shouldn't be"
        bundle = libernet.tools.bundle.Path(urls[0], storage)

        for url in reversed(urls[1:]):
            identifier, _, _ = libernet.tools.block.validate_url(url)
            block_path = os.path.join(storage, "upload", "local", "sha256", identifier[:libernet.tools.block.BLOCK_TOP_DIR_SIZE], identifier)
            shutil.rmtree(block_path)
            os.unlink(block_path + ".raw")
            assert sum([len(bundle.missing_blocks(f"{i:x}.txt")) for i in range(0, file_count)]) > 0

    libernet.tools.block.BLOCK_SIZE = old_block_size
    libernet.tools.bundle.MAX_BUNDLE_SIZE = old_block_size

def test_restore_index():
    data_contents = 'contents'
    urls = None

    with tempfile.TemporaryDirectory() as storage:
        with tempfile.TemporaryDirectory() as working:
            normal_path = os.path.join(working, 'normal.txt')
            libernet.plat.dirs.make_dirs(os.path.join(working, "empty dir"))

            with open(normal_path, 'w') as text_file:
                text_file.write(data_contents)
            
            urls = libernet.tools.bundle.create(working, storage, index="normal.txt")
            assert len(set(urls)) == len(urls)
        
        with tempfile.TemporaryDirectory() as restore:
            bundle = libernet.tools.bundle.Path(urls[0], storage)
            assert len(bundle.missing_blocks("")) == 0
            bundle.restore_file(restore, "")
            assert os.path.isfile(os.path.join(restore, "normal.txt"))

def test_modified_raws():
    data_contents = 'contents'
    urls = None

    with tempfile.TemporaryDirectory() as storage:
        with tempfile.TemporaryDirectory() as working:
            normal_path = os.path.join(working, 'normal.txt')
            libernet.plat.dirs.make_dirs(os.path.join(working, "empty dir"))

            with open(normal_path, 'w') as text_file:
                text_file.write(data_contents)
            
            urls = libernet.tools.bundle.create(working, storage, index="normal.txt")
            assert len(set(urls)) == len(urls)
        
        for root, _, files in os.walk(storage):
            for file in files:
                if os.path.splitext(file)[1] == '.raw':
                    with open(os.path.join(root, file), 'a') as f:
                        f.write('    ')

        with tempfile.TemporaryDirectory() as restore:
            bundle = libernet.tools.bundle.Path(urls[0], storage)
            assert len(bundle.missing_blocks("")) == 0
            try:
                bundle.restore_file(restore, "")
                assert False, "We should have thrown an exception"
            
            except ValueError:
                pass
