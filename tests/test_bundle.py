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

    def remove(self, key:str):
        del self.__contents[key]

    def keys(self):
        return self.__contents.keys()


def bundles_equal(b1, b2):
    assert len(b1.get('directories', [])) == len(b2.get('directories', [])), f"{b1.get('directories', [])} vs {b2.get('directories', [])}"
    assert len(b1.get('files', [])) == len(b2.get('files', [])), f"{b1.get('files', [])} vs {b2.get('files', [])}"
    assert set(b1.get('directories', [])) == set(b2.get('directories', [])), f"{set(b1.get('directories', []))} vs {set(b2.get('directories', []))}"
    assert set(b1.get('files', [])) == set(b2.get('files', [])), f"{set(b1.get('files', []))} vs {set(b2.get('files', []))}"

    for directory in b1.get('directories', []):
        assert b1['directories'][directory] == b2['directories'][directory], f"{directory}: {b1['directories'][directory]} vs {b2['directories'][directory]}"

    for file in b1.get('files', []):
        assert b1['files'][file]['size'] == b2['files'][file]['size'], f"{file}: {b1['files'][file]['size']} vs {b2['files'][file]['size']}"
        assert b1['files'][file].get('readonly', False) == b2['files'][file].get('readonly', False), f"{file}: {b1['files'][file].get('readonly', False)} vs {b2['files'][file].get('readonly', False)}"
        assert b1['files'][file].get('executable', False) == b2['files'][file].get('executable', False), f"{file}: {b1['files'][file].get('executable', False)} vs {b2['files'][file].get('executable', False)}"
        assert b1['files'][file].get('link', None) == b2['files'][file].get('link', None), f"{file}: {b1['files'][file].get('executable', False)} vs {b2['files'][file].get('executable', False)}"
        # TODO: fix date modified

        for block1, block2 in zip(b1['files'][file]['contents'], b2['files'][file]['contents']):
            assert block1['size'] == block2['size'], f"{file}: {block1} vs {block2}"
            id1 = block1['url'].split('/')[-1]
            id2 = block2['url'].split('/')[-1]
            assert id1 == id2, f"{file}: {block1} vs {block2}"


def makefile(path, contents):
    os.makedirs(os.path.split(path)[0], exist_ok=True)
    with open(path, "w") as file:
        file.write(contents)


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
        small_file_path = os.path.join(working_dir, 'small.txt')
        readonly_path = os.path.join(working_dir, 'readonly.txt')
        execute_path = os.path.join(working_dir, 'execute.txt')
        link_path = os.path.join(working_dir, 'link.txt')
        readonly_execute_path = os.path.join(working_dir, 'readonly execute.txt')
        os.mkdir(empty_dir_path)

        makefile(text_file_path, 'some text')
        makefile(small_file_path, 's')
        os.symlink('file.txt', link_path)
        makefile(readonly_path, 'read only')
        os.chmod(readonly_path, 0o444)
        makefile(execute_path, 'execute')
        os.chmod(execute_path, 0o777)
        makefile(readonly_execute_path, 'read only execute')
        os.chmod(readonly_execute_path, 0o555)
        url = libernet.bundle.create(working_dir, storage)

    restored = libernet.bundle.inflate(url, storage)
    assert restored['directories']['empty dir'] is None
    file_results = [
        ('execute.txt', True, False, 7, None, '/be6bdfe81ec4a456a537a4460f1f45e1fe33d30b4e220b0874299b82fd3a71e2'),
        ('readonly.txt', False, True, 9, None, '/1e5e3ce20c720a1021d8f9f5e8f7c3055a97315c79230508bbd37b08502208db'),
        ('readonly execute.txt', True, True, 17, None, '/6398f7bcf3282bf418a8c228ad15dd31ddf3a66712c82a0b10027f88507d4a2f'),
        ('file.txt', False, False, 9, None, '/b94f6f125c79e3a5ffaa826f584c10d52ada669e6762051b826b55776d05aed2'),
        ('small.txt', False, False, 1, None, '/043a718774c572bd8a25adbeb1bfcd5c0256ae11cecf9f9c3f925d0e52beaf89'),
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
            makefile(os.path.join(working_dir, f'file_{file_index}.txt'), f'file #{file_index}'*1000)

        url = libernet.bundle.create(working_dir, storage)

    restored = libernet.bundle.inflate(url, storage)
    assert len(restored['files']) == 100, f"{len(restored['files'])} {', '.join(restored['files'])}"
    assert 'bundles' not in restored
    assert 'directories' not in restored
    libernet.bundle.MAX_BLOCK_SIZE = old_block_max
    libernet.bundle.MAX_BUNDLE_SIZE = old_bundle_max


def test_missing_blocks():
    storage = Storage()
    old_block_max = libernet.bundle.MAX_BLOCK_SIZE
    old_bundle_max = libernet.bundle.MAX_BUNDLE_SIZE
    libernet.bundle.MAX_BLOCK_SIZE = 4096
    libernet.bundle.MAX_BUNDLE_SIZE = 4096

    with tempfile.TemporaryDirectory() as working_dir:
        for file_index in range(0,100):
            makefile(os.path.join(working_dir, f'file_{file_index}.txt'), f'file #{file_index}')

        url = libernet.bundle.create(working_dir, storage)

    restored = libernet.bundle.inflate(url, storage)
    subbundle_identifiers = {k.split('/')[2] for k in storage.keys()}
    root_identifier = url.split('/')[2]
    assert root_identifier in subbundle_identifiers
    subbundle_identifiers.remove(root_identifier)

    for file in restored['files']:
        file_identifiers = {c['url'].split('/')[2] for c in restored['files'][file]['contents']}
        assert file_identifiers.issubset(subbundle_identifiers)
        subbundle_identifiers -= file_identifiers

    assert len(subbundle_identifiers) == 6

    while subbundle_identifiers:
        subbundle_identifier = subbundle_identifiers.pop()
        subbundle_url = f"/sha256/{subbundle_identifier}"
        storage.remove(subbundle_url)
        partial_restore = libernet.bundle.inflate(url, storage)
        matches = any(u.startswith(subbundle_url) for u in partial_restore['bundles'])
        assert matches, f"{partial_restore['bundles']} vs {subbundle_url}"

    storage.remove(f'/sha256/{root_identifier}')
    no_restore = libernet.bundle.inflate(url, storage)
    assert no_restore is None, no_restore

    libernet.bundle.MAX_BLOCK_SIZE = old_block_max
    libernet.bundle.MAX_BUNDLE_SIZE = old_bundle_max


def test_previous():
    storage = Storage()

    with tempfile.TemporaryDirectory() as working_dir:
        file1_path = os.path.join(working_dir, 'file1.txt')
        file2_path = os.path.join(working_dir, 'file2.txt')
        file3_path = os.path.join(working_dir, 'file3.txt')
        makefile(file1_path, 'version 1')
        makefile(file3_path, 'version 3')
        url1 = libernet.bundle.create(working_dir, storage)
        previous = libernet.bundle.inflate(url1, storage)
        makefile(file2_path, 'version 2')
        makefile(file3_path, 'version 4')
        url2 = libernet.bundle.create(working_dir, storage, previous)
        current = libernet.bundle.inflate(url2, storage)
        previous_1_url = previous['files']['file1.txt']['contents'][0]['url']
        previous_3_url = previous['files']['file3.txt']['contents'][0]['url']
        current_1_url = current['files']['file1.txt']['contents'][0]['url']
        current_3_url = current['files']['file3.txt']['contents'][0]['url']
        assert previous_1_url == current_1_url
        assert previous_3_url != current_3_url
        assert 'file2.txt' in current['files']
        assert 'file2.txt' not in previous['files']


def test_unencrypted():
    storage = Storage()

    with tempfile.TemporaryDirectory() as working_dir:
        file1_path = os.path.join(working_dir, 'file1.txt')
        file2_path = os.path.join(working_dir, 'file2.txt')
        makefile(file1_path, 'version 1')
        makefile(file2_path, 'version 2')
        url = libernet.bundle.create(working_dir, storage, encrypt=False)
        assert len(url.split('/')) == 3, url
        restored = libernet.bundle.inflate(url, storage)
        assert len(restored['files']) == 2


def test_restore():
    storage = Storage()

    with (tempfile.TemporaryDirectory() as working_dir,
            tempfile.TemporaryDirectory() as destination_dir):
        dir1_path = os.path.join(working_dir, "dir1")
        dir2_path = os.path.join(working_dir, "dir2")
        dir3_path = os.path.join(working_dir, "dir3")
        dir4_path = os.path.join(working_dir, "dir3/dir4")
        file1_path = os.path.join(working_dir, "file1.txt")
        file2_path = os.path.join(working_dir, "dir2/file2.txt")
        file3_path = os.path.join(working_dir, "file3.txt")
        file4_path = os.path.join(working_dir, "dir3/dir4/file4.txt")
        link1_path = os.path.join(working_dir, "link1.txt")
        link2_path = os.path.join(working_dir, "link2")
        link3_path = os.path.join(working_dir, "dir2/link3.txt")
        link4_path = os.path.join(working_dir, "dir2/link4")
        os.makedirs(dir1_path, exist_ok=True)
        os.makedirs(dir2_path, exist_ok=True)
        os.makedirs(dir3_path, exist_ok=True)
        os.makedirs(dir4_path, exist_ok=True)
        makefile(file1_path, "file 1")
        makefile(file2_path, "file 2")
        makefile(file3_path, "file 3")
        makefile(file4_path, "file 4")
        os.symlink('file1.txt', link1_path)
        os.symlink('dir3/dir4', link2_path)
        os.symlink('../file3.txt', link3_path)
        os.symlink('../dir3/dir4/file4.txt', link4_path)
        os.chmod(file2_path, 0o444)  # readonly
        os.chmod(file3_path, 0o777)  # execute
        os.chmod(file4_path, 0o555)  # read only execute
        url1 = libernet.bundle.create(working_dir, storage)
        missing = libernet.bundle.restore(url1, destination_dir, storage)
        assert not missing, missing
        url2 = libernet.bundle.create(destination_dir, storage)

    restored1 = libernet.bundle.inflate(url1, storage)
    restored2 = libernet.bundle.inflate(url2, storage)
    bundles_equal(restored1, restored2)
    expectation = [
        ('file1.txt', 6, False, False, None),
        ('link1.txt', 6, False, False, "file1.txt"),
        ('file3.txt', 6, False, True, None),
        ('dir3/dir4/file4.txt', 6, True, True, None),
        ('dir2/link4', 6, True, True, "../dir3/dir4/file4.txt"),
        ('dir2/link3.txt', 6, False, True, "../file3.txt"),
        ('dir2/file2.txt', 6, True, False, None),
    ]
    assert restored2['directories']['dir1'] is None
    assert restored2['directories']['link2'] == "dir3/dir4", restored2['directories']['link2']
    assert len(restored2['directories']) == 2, ', '.join(restored2['directories'])
    assert len(restored2['files']) == 7, ', '.join(restored2['files'])

    for name, size, readonly, execute, link in expectation:
        assert restored2['files'][name]['size'] == size
        assert restored2['files'][name].get('readonly', False) == readonly
        assert restored2['files'][name].get('executable', False) == execute
        assert restored2['files'][name].get('link', None) == link


def test_restore_update():
    storage = Storage()

    with (tempfile.TemporaryDirectory() as working_dir,
            tempfile.TemporaryDirectory() as destination_dir):
        file1_path = os.path.join(working_dir, "file1.txt")
        file2_path = os.path.join(working_dir, "dir2/file2.txt")
        file3_path = os.path.join(working_dir, "file3.txt")
        file4_path = os.path.join(working_dir, "dir3/dir4/file4.txt")
        makefile(file1_path, "file 1")
        makefile(file2_path, "file 2")
        makefile(file3_path, "file 3")
        url1 = libernet.bundle.create(working_dir, storage)
        missing = libernet.bundle.restore(url1, destination_dir, storage)
        assert not missing, missing
        url2 = libernet.bundle.create(destination_dir, storage)
        makefile(file4_path, "file 4")
        os.remove(file3_path)
        makefile(file3_path, "file 5")
        url3 = libernet.bundle.create(working_dir, storage)
        missing = libernet.bundle.restore(url3, destination_dir, storage)
        assert not missing, missing
        url4 = libernet.bundle.create(destination_dir, storage)

    restored1 = libernet.bundle.inflate(url1, storage)
    restored2 = libernet.bundle.inflate(url2, storage)
    restored3 = libernet.bundle.inflate(url3, storage)
    restored4 = libernet.bundle.inflate(url4, storage)

    bundles_equal(restored1, restored2)
    bundles_equal(restored3, restored4)


def test_restore_update_2():
    storage = Storage()

    with (tempfile.TemporaryDirectory() as working_dir,
            tempfile.TemporaryDirectory() as destination_dir):
        working_dir, destination_dir = "/tmp/src", "/tmp/dst"
        file1_path = os.path.join(working_dir, "file1.txt")
        file2_path = os.path.join(working_dir, "dirA/file2.txt")
        dir1_path = os.path.join(working_dir, "dir1")
        dir2_path = os.path.join(working_dir, "dir2")

        makefile(file1_path, "file 1")
        os.makedirs(dir1_path, exist_ok=True)
        os.makedirs(os.path.join(destination_dir, "dirZ"), exist_ok=True)
        makefile(os.path.join(destination_dir, "dirY", "fileZ.txt"), "file Z")

        url1 = libernet.bundle.create(working_dir, storage)
        missing = libernet.bundle.restore(url1, destination_dir, storage)
        assert not missing, missing
        url2 = libernet.bundle.create(destination_dir, storage)

        os.rmdir(dir1_path)
        os.remove(file1_path)
        os.makedirs(dir2_path, exist_ok=True)
        makefile(file2_path, "file 2")

        url3 = libernet.bundle.create(working_dir, storage)
        missing = libernet.bundle.restore(url3, destination_dir, storage)
        assert not missing, missing
        url4 = libernet.bundle.create(destination_dir, storage)

    restored1 = libernet.bundle.inflate(url1, storage)
    restored2 = libernet.bundle.inflate(url2, storage)
    restored3 = libernet.bundle.inflate(url3, storage)
    restored4 = libernet.bundle.inflate(url4, storage)

    bundles_equal(restored1, restored2)
    bundles_equal(restored3, restored4)


if __name__ == "__main__":
    test_basic()
    test_file_metadata()
    test_large_bundle()
    test_missing_blocks()
    test_previous()
    test_unencrypted()
    test_restore()
    test_restore_update()
    test_restore_update_2()

