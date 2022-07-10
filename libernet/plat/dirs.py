#!/usr/bin/env python3

import os
import platform

def pref_dir(filename=None):
    home_dir = os.environ.get('HOME', '')
    appdata_dir = os.environ.get('APPDATA', '')
    directory = appdata_dir if platform.system() == 'Windows' else home_dir

    if platform.system() == 'Darwin':
        directory = os.path.join(directory, 'Library', 'Preferences')

    if not os.path.isdir(directory):
        os.makedirs(directory)

    if not directory or platform.system() == 'Linux' and filename:
        filename = '.' + filename

    return os.path.join(directory, filename) if filename else directory
