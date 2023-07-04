#!/usr/bin/env python3


import os
import stat
import tempfile

import libernet.bundle


class Storage:
    def __init__(self):
        self.__contents = {}

    def put(self, key_value:tuple):
        #print(f"put({key_value})")
        self.__contents.update(dict([key_value]))

    def get(self, key:str) -> bytes:
        #print(f"get({key})")
        return self.__contents.get(key, None)

    def has(self, key:str) -> bool:
        #print(f"has({key})")
        return key in self.__contents


def test_basic():
    path = os.path.realpath('tests')
    storage = Storage()
    url = libernet.bundle.create(path, storage)
    restored = libernet.bundle.inflate(url, storage)
    files = os.listdir('tests')

    for file in files:
        path = os.path.join('tests', file)

        if os.path.isfile(path):
            assert file in restored['files']
            assert os.path.getsize(path) == restored['files'][file]['size']


def test_file_metadata():
    storage = Storage()

    with tempfile.TemporaryDirectory() as working_dir:
        empty_dir_path = os.path.join(working_dir, 'empty dir')
        text_file_path = os.path.join(working_dir, 'file.txt')
        readonly_path = os.path.join(working_dir, 'readonly.txt')
        execute_path = os.path.join(working_dir, 'execute.txt')
        link_path = os.path.join(working_dir, 'link.txt')
        readonly_execute_path = os.path.join(working_dir, 'readonly execute.txt')
        os.mkdir(empty_dir_path)

        with open(text_file_path, 'w') as file:
            file.write('some text')

        os.symlink('file.txt', link_path)

        with open(readonly_path, 'w') as file:
            file.write('read only')

        os.chmod(readonly_path, 0o444)

        with open(execute_path, 'w') as file:
            file.write('execute')

        os.chmod(execute_path, 0o777)

        with open(readonly_execute_path, 'w') as file:
            file.write('read only execute')

        os.chmod(readonly_execute_path, 0o555)
        url = libernet.bundle.create(working_dir, storage)

    restored = libernet.bundle.inflate(url, storage)
    assert restored['directories']['empty dir'] == {}
    file_results = [
        ('execute.txt', True, False, 7, None, '/be6bdfe81ec4a456a537a4460f1f45e1fe33d30b4e220b0874299b82fd3a71e2'),
        ('readonly.txt', False, True, 9, None, '/1e5e3ce20c720a1021d8f9f5e8f7c3055a97315c79230508bbd37b08502208db'),
        ('readonly execute.txt', True, True, 17, None, '/6398f7bcf3282bf418a8c228ad15dd31ddf3a66712c82a0b10027f88507d4a2f'),
        ('file.txt', False, False, 9, None, '/b94f6f125c79e3a5ffaa826f584c10d52ada669e6762051b826b55776d05aed2'),
        ('link.txt', False, False, 9, "file.txt", '/b94f6f125c79e3a5ffaa826f584c10d52ada669e6762051b826b55776d05aed2'),
    ]

    for name, executable, readonly, size, link, identifier in file_results:
        assert executable == restored['files'][name].get('executable', False), f"{name}: {restored['files'][name]}"
        assert readonly == restored['files'][name].get('readonly', False), f"{name}: {restored['files'][name]}"
        assert restored['files'][name]['contents'][0]['url'].endswith(identifier), f"{name}: {restored['files'][name]}"
        assert restored['files'][name]['size'] == size, f"{name}: {restored['files'][name]}"
        assert link == restored['files'][name].get('link', None), f"{name}: {restored['files'][name]}"


def test_large_bundle():
    storage = Storage()
    old_block_max = libernet.bundle.MAX_BLOCK_SIZE
    old_bundle_max = libernet.bundle.MAX_BUNDLE_SIZE
    libernet.bundle.MAX_BLOCK_SIZE = 4096
    libernet.bundle.MAX_BUNDLE_SIZE = 4096

    with tempfile.TemporaryDirectory() as working_dir:
        for file_index in range(0,100):
            with open(os.path.join(working_dir, f'file_{file_index}.txt'), 'w') as file:
                file.write(f'file #{file_index}')

        url = libernet.bundle.create(working_dir, storage)

    restored = libernet.bundle.inflate(url, storage)
    assert len(restored['files']) == 100, f"{len(restored['files'])} {', '.join(restored['files'])}"
    assert 'bundles' not in restored
    assert 'directories' not in restored
    libernet.bundle.MAX_BLOCK_SIZE = old_block_max
    libernet.bundle.MAX_BUNDLE_SIZE = old_bundle_max


if __name__ == "__main__":
    test_basic()
    test_file_metadata()
    test_large_bundle()
