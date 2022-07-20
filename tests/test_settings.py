#!/usr/bin/env python3

import os
import tempfile

import libernet.tools.settings
import libernet.tools.block

def test_App():
    with tempfile.TemporaryDirectory() as storage:
        app1 = libernet.tools.settings.App(storage, key_size=1024)
        app2 = libernet.tools.settings.App(storage)
        assert app1.storage() == app2.storage()
        signature1 = app1.identity().sign_utf8("to sign")
        signature2 = app2.identity().sign_utf8("to sign")
        assert signature1 == signature2
        assert app1.identity().identifier() == app2.identity().identifier()
        identifier = app1.identity().identifier()
        block_path = libernet.tools.block.block_dir(os.path.join(storage, 'upload', 'local'), identifier, full=True) + '.raw'
        assert os.path.isfile(block_path)
        pem = libernet.tools.block.retrieve(f"/sha256/{app1.identity().identifier()}", storage)
        assert pem is not None

def test_sign_verify():
    with tempfile.TemporaryDirectory() as storage1:
        with tempfile.TemporaryDirectory() as storage2:
            app1 = libernet.tools.settings.App(storage1, key_size=1024)
            app2 = libernet.tools.settings.App(storage2, key_size=1024)
            headers = app1.sign_request('/sha256/abc')
            assert headers[libernet.tools.settings.HTTP_AUTHOR] == app1.identity().identifier()
            pem = libernet.tools.block.retrieve(f"/sha256/{app1.identity().identifier()}", storage1)
            libernet.tools.block.store_block(pem, storage2, encrypt=False)
            assert libernet.tools.settings.verify_request(headers, storage2)
            del headers[libernet.tools.settings.HTTP_AUTHOR]
            assert not libernet.tools.settings.verify_request(headers, storage2)
            del headers[libernet.tools.settings.HTTP_TIMESTAMP]
            assert not libernet.tools.settings.verify_request(headers, storage2)
