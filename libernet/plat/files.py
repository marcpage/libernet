""" Libernet platform specific directory tools
"""

import os
import platform

SYSTEM = platform.system()
RSRC_FORK = os.path.join("..namedfork", "rsrc")


def symlink(src, dst):
    """create a symlink if os supports it
    src the path to write into the link file at dst
    dst the symlink to create
    returns True if a symlink was created, False if not
    """
    if SYSTEM == "Windows":
        return False

    os.symlink(src, dst)
    return True


def open_url(url):
    """opens the default browser (if supported)"""
    if SYSTEM == "Darwin":  # NOT TESTED
        os.system(f'open "{url}"')

    elif SYSTEM == "Windows":  # NOT TESTED
        os.system(f'explorer "{url}"')

    elif SYSTEM == "Linux":  # NOT TESTED
        os.system(f'xdg-open "{url}"')


def rsrc_fork_path(path, verify=True):
    """Get the path to the resource fork for the file if it exists, None otherwise"""
    if SYSTEM == "Darwin":  # NOT TESTED
        rsrc_path = os.path.join(path, RSRC_FORK)
        return rsrc_path if not verify or os.path.isfile(rsrc_path) else None

    return None
