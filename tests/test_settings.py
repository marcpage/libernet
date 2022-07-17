#!/usr/bin/env python3

import tempfile

import libernet.tools.settings

def test_App():
    with tempfile.TemporaryDirectory() as storage:
        app1 = libernet.tools.settings.App(storage, key_size=1024)
        app2 = libernet.tools.settings.App(storage)
        assert app1.storage() == app2.storage()
        signature1 = app1.identity().sign(libernet.tools.hash.sha256_hasher(b"to sign"))
        signature2 = app2.identity().sign(libernet.tools.hash.sha256_hasher(b"to sign"))
        assert signature1 == signature2
