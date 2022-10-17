#!/usr/bin/env python3
import tempfile
import multiprocessing
import os
import time
import sys
import signal

import requests

import libernet.server
import libernet.tools.bundle
import libernet.tools.block

def test_arg_parser():
        parser = libernet.server.get_arg_parser()
        assert parser is not None


def __asked_to_quit(*args):
    print("Exiting gracefully")
    sys.exit(0)


def __run_server(port:int, storage:str, debug:bool, key_size:int, fake_remote:bool):
    """ This test code needs to be here to be able to set the local machine test value """
    args = type('',(),{})
    args.port = port
    args.storage = storage
    args.debug = debug
    testing = libernet.plat.network.TEST_NOT_LOCAL_MACHINE
    libernet.plat.network.TEST_NOT_LOCAL_MACHINE = fake_remote
    signal.signal(signal.SIGINT, __asked_to_quit)
    signal.signal(signal.SIGTERM, __asked_to_quit)
    libernet.server.handle_args(args, key_size=key_size)
    libernet.plat.network.TEST_NOT_LOCAL_MACHINE = testing
    

def test_server_local():
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

        server = multiprocessing.Process(
            target=__run_server,
            args=(port_to_use, storage, debug, key_size, False),
            daemon=True,
        )
        server.start()

        start = time.time()
        bundle1 = urls1[0]
        bundle2 = urls2[0]
        url3 = libernet.tools.block.store_block(contents=b' testing ' * 50, storage=storage, encrypt=False)
        url4 = libernet.tools.block.store_block(contents=b' testing ', storage=storage, encrypt=False)
        url4_parts = libernet.tools.block.validate_url(url4)
        search_dirs = libernet.tools.block.get_search_dirs(storage=storage)

        for search_dir in search_dirs:
            block_path = libernet.tools.block.block_dir(search_dir, url4_parts[0], full=True)
            block_file = block_path + '.raw'

            if os.path.isfile(block_file):
                with open(block_file, 'a') as block_file:
                        block_file.write(" corupted ")

        not_bundle = urls1[0][:-4] + 'FFFF'
        block1 = f"/{bundle1.split('/')[1]}/{bundle1.split('/')[2]}"
        block2 = f"/{bundle2.split('/')[1]}/{bundle2.split('/')[2]}"

        while True:
            try:
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

                response = requests.get(f'http://localhost:{port_to_use}{block2}')
                assert response.status_code == 200
                assert len(response.content) <= len(test_contents)

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

                response = requests.get(f'http://localhost:{port_to_use}{url3}')
                assert response.status_code == 200

                response = requests.get(f'http://localhost:{port_to_use}{url4}')
                assert response.status_code == 400  # bad request

                response = requests.get(f'http://localhost:{port_to_use}{not_bundle}/test.html')
                assert response.status_code == 409  # Conflict

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
                assert time.time() - start < 5.0  # max 5 seconds
                sys.stdout.write(f'{time.time() - start:0.0f} ')
                sys.stdout.flush()
                time.sleep(1.0)  # wait for the Flask app server to cme up

        server.terminate()
        server.join()


def test_server_remote():
    port_to_use = 8087
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

        server = multiprocessing.Process(
            target=__run_server,
            args=(port_to_use, storage, debug, key_size, True),
            daemon=True,
        )
        server.start()

        start = time.time()
        bundle1 = urls1[0]
        bundle2 = urls2[0]
        url4 = libernet.tools.block.store_block(contents=b' testing ', storage=storage, encrypt=False)
        url4_parts = libernet.tools.block.validate_url(url4)
        search_dirs = libernet.tools.block.get_search_dirs(storage=storage)

        for search_dir in search_dirs:
            block_path = libernet.tools.block.block_dir(search_dir, url4_parts[0], full=True)
            block_file = block_path + '.raw'

            if os.path.isfile(block_file):
                with open(block_file, 'a') as block_file:
                        block_file.write(" corupted ")

        block1 = f"/{bundle1.split('/')[1]}/{bundle1.split('/')[2]}"
        block2 = f"/{bundle2.split('/')[1]}/{bundle2.split('/')[2]}"

        while True:
            try:
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

                response = requests.get(f'http://localhost:{port_to_use}{block2}')
                assert response.status_code == 200

                response = requests.get(f'http://127.0.0.1:{port_to_use}/api/v1/backup/add')
                assert response.status_code == 403  # forbidden

                response = requests.get(f'http://127.0.0.1:{port_to_use}/api/v1/backup/remove')
                assert response.status_code == 403  # forbidden

                response = requests.get(f'http://127.0.0.1:{port_to_use}/api/v1/backup/list')
                assert response.status_code == 403  # forbidden

                # add more remote requests here

                break

            except requests.exceptions.ConnectionError:
                assert time.time() - start < 5.0  # max 5 seconds
                sys.stdout.write(f'{time.time() - start:0.0f} ')
                sys.stdout.flush()
                time.sleep(1.0)  # wait for the Flask app server to cme up

        server.terminate()
        server.join()

if __name__ == "__main__":  # NOT TESTED
    test_server_local()
    test_server_remote()
