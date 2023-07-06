#!/usr/bin/env python3


import tempfile
import multiprocessing
import os
import time
import sys
import signal
import hashlib
import json

from random import randbytes
from types import SimpleNamespace

import requests

import libernet.server

from libernet.hash import sha256_data_identifier, identifier_match_score


DATA_SIZE = 30

def test_arg_parser():
        parser = libernet.server.get_arg_parser()
        assert parser is not None


def write_and_read_data(client, data:bytes):
    identifier = sha256_data_identifier(data)
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
    return identifier


def test_app():
    max_like = libernet.server.MAX_LIKE
    libernet.server.MAX_LIKE = 5
    port_to_use = 8086
    debug = False
    args = SimpleNamespace(storage=None, debug=debug, port=port_to_use)

    with tempfile.TemporaryDirectory() as storage:
        import os
        storage = os.path.join("/tmp", os.path.split(storage)[1])
        args.storage = storage
        instance = libernet.server.create_app(args)
        test_set = [
            b'',
            b'hello',
            '😀😔'.encode('utf-8'),

            b'XTz\xef\x86\x8cD\x86rV\x03\x1b\xa0*\xdeS\x1d\x03,\xe6\x1f1\x919\xdfR\x1eaB\x12',
            b't\x14DY#\xb5\xa3\x7f\xbeifQM\xa8n/\xc7\xa9tdB?\xde\xadcWr%\x10\xaf',
            b'\xf5\xcc\xa2\rW\x1av\xe0\x8a5{\n\x11\xb5B\x97D\xde\xc1\xb3\xd7x]\xaf\x0668\xb2j+',
            b'/\x94p\xcdv\x99\x9dK\x14>\xde\x0c\x06\xf7\xa7YG\xd6\xaf\x1a\xfad_\xc5\xf9\xc2i\x19\xf1[',
            b',lp\xd1\x7f\x05\x0f\x972\xa1KY\x16\x0b\x1c\x80!%\x84\xfdO\xf9\x0b3\x056\x18\xcb\xf8R',
            b'Ts0\x1fh\x15y[\xe4O\xb7Y\xbc\xbc\x7f\x957HAG<3\xb4\xe1\x10\xb4N\x92\x9b\xe3',
            b'3z(\xad\xe6m\x9ah\x15\xed\x91\xb2|\x0e=\xf5\x9e%\xfez \xb36\x1a\x90\xfe\x83\xb8\xde\xbf',

            b'\ny2f"\xb4\xb1A\xe8\xf8\xf8c\x0ep(\xdb7\xb3\x16\x97\xf8\xc2\xb9\xd0\xbex\x1fc\x87\xc0',
            b'\xde\x83W\xe2\xd3\xd0~\x0f\xfd\xccaE\x9f1\x041\xe7\x14\x00m\x8a\x08\xbf]?>a\x15\xd0\xd3',
            b'\xc8\x1dA`V\xc10\x0f\xfa\xc35\xf8>\x11\xb1\xd0\x86\xed\x19\x0f\xbb\x91\r\xdbKb5U\xad\x16',
            b'(\xf2\x8ei\x91\xae\xf6\xe4\xc3+\x1d\xedq\xb5\xeaN\xf3\xd2ms\xc2\x9dm\xcfn\x11\x1c\xd5\xdc\x16',
            b'\x92\x83\xe6?`\x92\xf1\xe5\xb5#\x9d\x84z\xb7\xd4\xce\x02\xed7\xf6Fw\xb1kL\x1f\xeb\xacx\x0b',

            b'`\xde3\xb28\xe3\nou\xde\x03\xf0gy\xc6\x96&\xf8j\xf1\x9e\xee|\xd1s\xdb\xe6\xbd`2',
            b'(\xad\x06s:\xbb\xb5tlSh\x91yP\x1f\r\xd4\x12jM[\x9ca\x81\xce<L"6\x00',
            b'\xc3\x1fp\xc4\\4|&\xfb\xb1V*\xc1\x81\xa4\x1e\x03\x04q\xad(\x1b.\xc2;\x9f\xda\x9a\xa1\x98',
            b'\x0e\xac4\xb0\xa1\xd4\xd3\xabk\xc2f\xc7\xef9\xe75\xa0\xc6\xe5\x82+\x01\xab\xf7\x10o<\xee\xdcw',
            b"\x15QU\xf9\xff\x17[oE\x17\xbd\xe2b\x1aNo\xc4\x9cc\x9f\xada\xae\x9f\xb9ph'\xa8\xaa",
        ]

        with instance.test_client() as test_client:
            identifiers = [write_and_read_data(test_client, d) for d in test_set]
            found_at_max = False
            identifier = sha256_data_identifier(b'no way')
            response = test_client.get(f'/sha256/like/{identifier}')
            assert response.status_code == 404, f"{response.status_code} {response.data}"
            found = json.loads(response.data.decode('utf-8'))
            assert found == {}, found

            while identifiers:
                identifier = identifiers.pop()
                response = test_client.get(f'/sha256/like/{identifier}')
                assert response.status_code == 200, f"{response.status_code} {response.data}"
                found = json.loads(response.data.decode('utf-8'))
                assert identifier in found
                assert len(found) <= libernet.server.MAX_LIKE
                found_at_max = found_at_max or len(found) == libernet.server.MAX_LIKE

                for matching in found:
                    if matching in identifiers:
                        identifiers.remove(matching)

    assert found_at_max
    libernet.server.MAX_LIKE = max_like


if __name__ == "__main__":
    test_arg_parser()
    test_app()
