#!/usr/bin/env python3


import time

from tempfile import TemporaryDirectory
from types import SimpleNamespace
from multiprocessing import Process

from libernet.server import serve
from libernet.block import store, fetch, address
from libernet.proxy import Storage
from libernet.hash import sha256_data_identifier


SERVER_PORT = 4242
STARTUP_WAIT = 0.200  # seconds
SHUTDOWN_WAIT = 0.100  # seconds


def test_basics():
    test_set = [
        b'XTz\xef\x86\x8cD\x86rV\x03\x1b\xa0*\xdeS\x1d\x03,\xe6\x1f1\x919\xdfR\x1eaB\x12',
        b't\x14DY#\xb5\xa3\x7f\xbeifQM\xa8n/\xc7\xa9tdB?\xde\xadcWr%\x10\xaf',
        b'\xf5\xcc\xa2\rW\x1av\xe0\x8a5{\n\x11\xb5B\x97D\xde\xc1\xb3\xd7x]\xaf\x0668\xb2j+',
        b'/\x94p\xcdv\x99\x9dK\x14>\xde\x0c\x06\xf7\xa7YG\xd6\xaf\x1a\xfad_\xc5\xf9\xc2i\x19\xf1[',
        b',lp\xd1\x7f\x05\x0f\x972\xa1KY\x16\x0b\x1c\x80!%\x84\xfdO\xf9\x0b3\x056\x18\xcb\xf8R',
        b'Ts0\x1fh\x15y[\xe4O\xb7Y\xbc\xbc\x7f\x957HAG<3\xb4\xe1\x10\xb4N\x92\x9b\xe3',
        b'3z(\xad\xe6m\x9ah\x15\xed\x91\xb2|\x0e=\xf5\x9e%\xfez \xb36\x1a\x90\xfe\x83\xb8\xde\xbf',
    ]

    with TemporaryDirectory() as working_dir:
        server = Process(target=serve,
                args=(SimpleNamespace(storage=working_dir, port=SERVER_PORT, debug=False),))
        server.start()
        time.sleep(STARTUP_WAIT)  # wait for the server to come up
        proxy = Storage("localhost", SERVER_PORT)
        expected = {}
        test_set.extend(('testing '*c).encode('utf-8') for c in range(0, 100))
        max_found = 0

        for key in test_set:
            expected[key] = store(key, proxy, encrypt=False)

        for key in expected:
            assert address(expected[key][0]) in proxy

        for key in expected:
            assert proxy[address(expected[key][0])] == expected[key][1]

        for key in expected:
            found_value = fetch(expected[key][0], proxy)
            assert key == found_value, f"{key} vs {found_value}"

        for key in expected:
            found = proxy.like(address(expected[key][0]))
            assert address(expected[key][0]) in found, f"{address(expected[key][0])} vs {found}"
            assert len(found) <= 7, found
            max_found = max(max_found, len(found))

        proxy.shutdown()
        proxy.join()  # wait for any pending messages to be sent
        server.kill()
        time.sleep(SHUTDOWN_WAIT)  # wait for the server to shutdown


def test_errors():
    missing_identifier = sha256_data_identifier(b'no way')

    with TemporaryDirectory() as working_dir:
        server = Process(target=serve,
                args=(SimpleNamespace(storage=working_dir, port=SERVER_PORT, debug=False),))
        server.start()
        time.sleep(STARTUP_WAIT)  # wait for the server to come up
        proxy = Storage("localhost", SERVER_PORT)
        missing_results = proxy.like(f"/sha256/{missing_identifier}")
        assert missing_results == {}, missing_results
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
