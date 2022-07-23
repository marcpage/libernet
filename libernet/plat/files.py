""" Libernet platform specific directory tools
"""

import os
import platform

SYSTEM = platform.system()


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
