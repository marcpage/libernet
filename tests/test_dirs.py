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
