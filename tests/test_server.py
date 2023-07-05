#!/usr/bin/env python3


import tempfile
import multiprocessing
import os
import time
import sys
import signal
from types import SimpleNamespace
import hashlib

import requests

import libernet.server


def test_arg_parser():
        parser = libernet.server.get_arg_parser()
        assert parser is not None


def write_and_read_data(client, data:bytes):
    identifier = hashlib.sha256(data).hexdigest()
    response = client.get(f'/sha256/{identifier}')
    assert response.status_code == 504
    response = client.head(f'/sha256/{identifier}')
    assert response.status_code != 404
    response = client.put(f"/sha256/{identifier}", data=data)
    assert response.status_code == 200, f"'{data}' -> localhost:{port} -> {response}"
    response = client.get(f'/sha256/{identifier}')
    assert response.status_code == 200
    assert response.data == data
    response = client.head(f'/sha256/{identifier}')
    assert response.status_code == 200


def test_app():
    port_to_use = 8086
    debug = False
    args = SimpleNamespace(storage=None, debug=debug, port=port_to_use)

    with tempfile.TemporaryDirectory() as storage:
        args.storage = storage
        instance = libernet.server.create_app(args)

        with instance.test_client() as test_client:
            write_and_read_data(test_client, b'')
            write_and_read_data(test_client, b'hello')
            write_and_read_data(test_client, '😀😔'.encode('utf-8'))


if __name__ == "__main__":
    test_arg_parser()
    test_app()
