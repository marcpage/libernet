#!/usr/bin/env python3

""" Libernet platform specific directory tools
"""

import os
import platform


def make_dirs(path):
    """make directories if they don't exist"""
    try:
        os.makedirs(path)

    except FileExistsError:
        pass


def pref_dir(filename=None):
    """Get the preferences directory"""
    home_dir = os.environ.get("HOME", "")
    appdata_dir = os.environ.get("APPDATA", "")
    directory = appdata_dir if platform.system() == "Windows" else home_dir

    if platform.system() == "Darwin":
        directory = os.path.join(directory, "Library", "Preferences")

    if not os.path.isdir(directory):
        make_dirs(directory)

    if (not directory or platform.system() == "Linux") and filename:
        filename = "." + filename

    return os.path.join(directory, filename) if filename else directory


def path_relative_to(path, base):
    """Given path which resides in base, return portion of path relative to base
    returns relative path from base to path
    """
    parts = []
    base = base.rstrip('/')
    while path != base:
        (path, name) = os.path.split(path)
        parts.append(name)
    return os.path.join(*reversed(parts)) if len(parts) > 0 else ""
