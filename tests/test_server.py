#!/usr/bin/env python3
import tempfile
import threading
import os
import time
import sys

import requests

import libernet.server
import libernet.tools.bundle


def test_arg_parser():
        parser = libernet.server.get_arg_parser()
        help = parser.format_help()
        assert "Libernet" in help


def test_server_bundle_index():
    port_to_use = 8086
    key_size = 1024
    debug = False
    index_contents = b"<html><body>" + b" index " * 100 + b"</body></html>"
    test_contents = b"<html><body>" + b" test " * 100 + b"</body></html>"

    with tempfile.TemporaryDirectory() as storage:
        with tempfile.TemporaryDirectory() as working:
            with open(os.path.join(working, "index.html"), 'wb') as f:
                f.write(index_contents)
            
            urls1 = libernet.tools.bundle.create(working, storage, index="index.html")

        with tempfile.TemporaryDirectory() as working:
            with open(os.path.join(working, "test.html"), 'wb') as f:
                f.write(test_contents)
            
            urls2 = libernet.tools.bundle.create(working, storage)
        
        threading.Thread(
            target=libernet.server.serve,
            args=(port_to_use, storage, debug, key_size),
            daemon=True,
        ).start()

        start = time.time()
        bundle1 = urls1[0]
        bundle2 = urls2[0]
        block1 = f"/{bundle1.split('/')[1]}/{bundle1.split('/')[2]}"
        block2 = f"/{bundle2.split('/')[1]}/{bundle2.split('/')[2]}"
        local_machine = libernet.plat.network.TEST_NOT_LOCAL_MACHINE

        while True:
            try:
                # remote tests
                libernet.plat.network.TEST_NOT_LOCAL_MACHINE = True

                response = requests.get(f'http://localhost:{port_to_use}{bundle1}/')
                assert response.status_code == 403  # forbidden
                assert response.content != index_contents

                response = requests.get(f'http://localhost:{port_to_use}/')
                assert response.status_code == 403  # forbidden

                response = requests.get(f'http://127.0.0.1:{port_to_use}/')
                assert response.status_code == 403  # forbidden

                response = requests.get(f'http://localhost:{port_to_use}{bundle1}')
                assert response.status_code == 403  # forbidden

                response = requests.get(f'http://localhost:{port_to_use}{bundle1}/not_found.html')
                assert response.status_code == 403  # forbidden

                response = requests.get(f'http://localhost:{port_to_use}{bundle2}')
                assert response.status_code == 403  # forbidden

                response = requests.get(f'http://localhost:{port_to_use}{bundle2}/not_found.html')
                assert response.status_code == 403  # forbidden

                response = requests.get(f'http://localhost:{port_to_use}{block1}')
                assert response.status_code == 200
                block1_contents = response.content

                response = requests.get(f'http://localhost:{port_to_use}{block2}')
                assert response.status_code == 200
                block2_contents = response.content

                response = requests.get(f'http://127.0.0.1:{port_to_use}/api/v1/backup/add')
                assert response.status_code == 403  # forbidden

                response = requests.get(f'http://127.0.0.1:{port_to_use}/api/v1/backup/remove')
                assert response.status_code == 403  # forbidden

                response = requests.get(f'http://127.0.0.1:{port_to_use}/api/v1/backup/list')
                assert response.status_code == 403  # forbidden

                # add more remote requests here


                # local tests
                libernet.plat.network.TEST_NOT_LOCAL_MACHINE = False

                response = requests.get(f'http://localhost:{port_to_use}{bundle1}/')
                assert response.status_code == 200, f'http://localhost:{port_to_use}{bundle1}/ => {response.status_code}'
                assert response.content == index_contents

                response = requests.get(f'http://localhost:{port_to_use}{bundle1}/index.html')
                assert response.status_code == 200
                assert response.content == index_contents

                response = requests.get(f'http://localhost:{port_to_use}{bundle1}/not_found.html')
                assert response.status_code == 404

                response = requests.get(f'http://localhost:{port_to_use}{bundle1}')
                assert response.status_code == 200

                response = requests.get(f'http://localhost:{port_to_use}{block1}')
                assert response.status_code == 200
                assert len(response.content) <= len(index_contents)
                assert response.content == block1_contents

                response = requests.get(f'http://localhost:{port_to_use}{block2}')
                assert response.status_code == 200
                assert len(response.content) <= len(test_contents)
                assert response.content == block2_contents

                response = requests.get(f'http://localhost:{port_to_use}/')
                assert response.status_code == 200

                response = requests.get(f'http://127.0.0.1:{port_to_use}/')
                assert response.status_code == 200
                
                response = requests.get(f'http://localhost:{port_to_use}{bundle2}/test.html')
                assert response.status_code == 200
                assert response.content == test_contents

                response = requests.get(f'http://localhost:{port_to_use}{bundle2}/not_found.html')
                assert response.status_code == 404
                
                response = requests.get(f'http://localhost:{port_to_use}{bundle2}/')
                assert response.status_code == 404  # File not found

                response = requests.get(f'http://127.0.0.1:{port_to_use}/api/v1/backup/add')
                assert response.status_code == 200
                assert response.content == b'{}'

                response = requests.get(f'http://127.0.0.1:{port_to_use}/api/v1/backup/remove')
                assert response.status_code == 200
                assert response.content == b'{}'

                response = requests.get(f'http://127.0.0.1:{port_to_use}/api/v1/backup/list')
                assert response.status_code == 200
                assert response.content == b'[]'


                # add more local requests here

                break
            
            except requests.exceptions.ConnectionError:
                assert time.time() - start < 30  # typically 5 - 20 seconds
                sys.stdout.write(f'{time.time() - start:0.0f} ')
                sys.stdout.flush()
                time.sleep(1.0)  # wait for the Flask app server to cme up

        libernet.plat.network.TEST_NOT_LOCAL_MACHINE = local_machine
