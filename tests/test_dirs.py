#!/usr/bin/env python3

import tempfile
import os

import libernet.plat.dirs


def test_make_dirs():
    with tempfile.TemporaryDirectory() as storage:
        directory_to_create = os.path.join(storage, "test", "me", "now")
        libernet.plat.dirs.make_dirs(directory_to_create)
        assert os.path.isdir(directory_to_create)
        libernet.plat.dirs.make_dirs(directory_to_create)


def test_pref_dir():
    assert os.path.isdir(libernet.plat.dirs.pref_dir())
    assert not os.path.isfile(libernet.plat.dirs.pref_dir("a preference name that should never exist.bak"))

def test_path_relative_to():
    base = os.path.join("test", "me", "now/")
    full = os.path.join(base, "test", "more")
    assert libernet.plat.dirs.path_relative_to(full, base) == os.path.join("test", "more")

def test_pref_dir_params(system=None, home=None, appdata=None, path=None, filename=None):
    if system is None:
        return

    previous = {
        'home': libernet.plat.dirs.HOME_DIR,
        'appdata': libernet.plat.dirs.APPDATA_DIR,
        'system': libernet.plat.dirs.SYSTEM,
    }
    libernet.plat.dirs.SYSTEM = system
    libernet.plat.dirs.HOME_DIR = home
    libernet.plat.dirs.APPDATA_DIR = appdata
    assert libernet.plat.dirs.pref_dir(filename) == path
    assert os.path.isdir(libernet.plat.dirs.pref_dir())
    libernet.plat.dirs.HOME_DIR = previous['home']
    libernet.plat.dirs.APPDATA_DIR = previous['appdata']
    libernet.plat.dirs.SYSTEM = previous['system']

def test_pref_dir_mac():
    with tempfile.TemporaryDirectory() as home:
        with tempfile.TemporaryDirectory() as appdata:
            test_pref_dir_params("Darwin", home, appdata, os.path.join(home, 'Library', 'Preferences'))

def test_pref_dir_mac_file():
    with tempfile.TemporaryDirectory() as home:
        with tempfile.TemporaryDirectory() as appdata:
            test_pref_dir_params("Darwin", home, appdata, os.path.join(home, 'Library', 'Preferences', 'test.txt'), "test.txt")

def test_pref_dir_linux():
    with tempfile.TemporaryDirectory() as home:
        with tempfile.TemporaryDirectory() as appdata:
            lin_home = os.path.join(home, 'nonexistent')
            test_pref_dir_params("Linux", lin_home, appdata, lin_home)

def test_pref_dir_linux_file():
    with tempfile.TemporaryDirectory() as home:
        with tempfile.TemporaryDirectory() as appdata:
            lin_home = os.path.join(home, 'nonexistent')
            test_pref_dir_params("Linux", lin_home, appdata, os.path.join(lin_home, '.test.txt'), "test.txt")

def test_pref_dir_windows():
    with tempfile.TemporaryDirectory() as home:
        with tempfile.TemporaryDirectory() as appdata:
            test_pref_dir_params("Windows", home, appdata, appdata)

def test_pref_dir_windows_file():
    with tempfile.TemporaryDirectory() as home:
        with tempfile.TemporaryDirectory() as appdata:
            test_pref_dir_params("Windows", home, appdata, os.path.join(appdata, 'test.txt'), "test.txt")

