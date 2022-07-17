#!/usr/bin/env python3

""" Libernet timestamp management
"""

import time
import datetime

UTC_TIMEZONE = datetime.timezone(datetime.timedelta(0))
TIMESTAMP_EPOCH = datetime.datetime(2001, 1, 1, tzinfo=UTC_TIMEZONE).timestamp()


def create(py_time=None):
    """Creates a timestamp (optionally from a python-epoch based time"""
    py_time = time.time() if py_time is None else py_time
    return py_time - TIMESTAMP_EPOCH


def convert(timestamp):
    """Convert a timestamp back into python-epoch based time"""
    return timestamp + TIMESTAMP_EPOCH
