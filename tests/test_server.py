#!/usr/bin/env python3
import tempfile
import threading
import os
import time
import sys

import requests

import libernet.server
import libernet.tools.bundle


def test_server_bundle_index():
    port_to_use = 8086
    index_contents = b"<html><body>index</body></html>"

    with tempfile.TemporaryDirectory() as storage:
        with tempfile.TemporaryDirectory() as working:
            
            with open(os.path.join(working, "index.html"), 'wb') as f:
                f.write(index_contents)
            
            urls = libernet.tools.bundle.create(working, storage, index="index.html")
        
        threading.Thread(
            target=libernet.server.serve,
            args=(port_to_use, storage, False),
            daemon=True,
        ).start()

        start = time.time()

        while True:
            try:
                response = requests.get(f'http://localhost:{port_to_use}{urls[0]}/')
                assert response.status_code == 200
                assert response.content == index_contents
                break
            
            except requests.exceptions.ConnectionError:
                assert time.time() - start < 30
                sys.stdout.write(f'{time.time() - start:0.0f} ')
                sys.stdout.flush()
                time.sleep(1)
