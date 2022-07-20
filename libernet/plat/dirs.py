#!/usr/bin/env python3

""" Libernet platform specific directory tools
"""

import os
import platform

HOME_DIR = os.environ.get("HOME", "")
APPDATA_DIR = os.environ.get("APPDATA", "")
SYSTEM = platform.system()


def make_dirs(path):
    """make directories if they don't exist"""
    try:
        os.makedirs(path)

    except FileExistsError:
        pass


def pref_dir(filename=None):
    """Get the preferences directory"""
    directory = APPDATA_DIR if SYSTEM == "Windows" else HOME_DIR

    if SYSTEM == "Darwin":
        directory = os.path.join(directory, "Library", "Preferences")

    make_dirs(directory)

    if (not directory or SYSTEM == "Linux") and filename:
        filename = "." + filename

    return os.path.join(directory, filename) if filename else directory


def path_relative_to(path, base):
    """Given path which resides in base, return portion of path relative to base
    returns relative path from base to path
    """
    parts = []
    base = base.rstrip("/")
    while path != base:
        (path, name) = os.path.split(path)
        parts.append(name)
    return os.path.join(*reversed(parts)) if len(parts) > 0 else ""
