#!/usr/bin/env python3

import time

from tempfile import TemporaryDirectory
from types import SimpleNamespace
from multiprocessing import Process

from libernet.server import serve
from libernet.block import store, fetch, address
from libernet.proxy import Storage

SERVER_PORT = 4242
STARTUP_WAIT = 0.200  # seconds
SHUTDOWN_WAIT = 0.100  # seconds


def test_basics():
    with TemporaryDirectory() as working_dir:
        server = Process(target=serve,
                args=(SimpleNamespace(storage=working_dir, port=SERVER_PORT, debug=False),))
        server.start()
        time.sleep(STARTUP_WAIT)  # wait for the server to come up
        proxy = Storage("localhost", SERVER_PORT)
        expected = {}

        for word_count in range(0, 100):
            key = ('testing '*word_count).encode('utf-8')
            expected[key] = store(key, proxy)

        for key in expected:
            assert address(expected[key][0]) in proxy

        for key in expected:
            assert proxy[address(expected[key][0])] == expected[key][1]

        for key in expected:
            found_value = fetch(expected[key][0], proxy)
            assert key == found_value, f"{key} vs {found_value}"

        proxy.shutdown()
        proxy.join()  # wait for any pending messages to be sent
        server.kill()
        time.sleep(SHUTDOWN_WAIT)  # wait for the server to shutdown


def test_errors():
    with TemporaryDirectory() as working_dir:
        import os
        working_dir = os.path.join("/tmp", os.path.split(working_dir)[1])
        server = Process(target=serve,
                args=(SimpleNamespace(storage=working_dir, port=SERVER_PORT, debug=False),))
        server.start()
        time.sleep(STARTUP_WAIT)  # wait for the server to come up
        proxy = Storage("localhost", SERVER_PORT)
        proxy['/hello'] = b'oh, no'
        storage = {}
        url1, _ = store(b'hello', storage, encrypt=False)
        assert address(url1) not in proxy, f"{address(url1)} <- {url1}"
        assert proxy.get(address(url1)) is None
        try:
            value = proxy[address(url1)]
            assert False, f"We got {value} but should not have"
        except KeyError:
            pass
        url2, _ = store(b'hello', proxy, encrypt=False)
        assert url1 == url2
        assert address(url1) in proxy, address(url1)
        assert address(url2) in proxy, address(url2)
        proxy.shutdown()
        proxy.join()  # wait for any pending messages to be sent
        server.kill()
        time.sleep(SHUTDOWN_WAIT)  # wait for the server to shutdown


if __name__ == "__main__":
    test_basics()
    test_errors()
