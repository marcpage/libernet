#!/usr/bin/env python3


from tempfile import TemporaryDirectory

import libernet.disk

from libernet.disk import Storage
from libernet.block import store, fetch, address_of
from libernet.hash import sha256_data_identifier


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
        storage = Storage(working_dir)
        expected = {}
        test_set.extend(('testing '*c).encode('utf-8') for c in range(0, 100))
        max_found = 0

        for key in test_set:
            expected[key] = store(key, storage, encrypt=False)

        for key in expected:
            assert address_of(expected[key][0]) in storage

        for key in expected:
            assert storage[address_of(expected[key][0])] == expected[key][1]

        for key in expected:
            found_value = fetch(expected[key][0], storage)
            assert key == found_value, f"{key} vs {found_value}"

        for key in expected:
            found = storage.like(address_of(expected[key][0]))
            assert address_of(expected[key][0]) in found, f"{address_of(expected[key][0])} vs {found}"
            assert len(found) <= 7, found
            max_found = max(max_found, len(found))


def test_corners():
    external = {"/sha256/eca8d73d099dfebcbf3e20a57935101839ac2c7739ccc9f2dd36d77fb8b54212": 30, "/sha256/ecabb29bbec22a6501c06aec21d8b36adfbaecd7fd57389a77563938d57ae2be": 30, "/sha256/ecabaf6cf7d763f63a9a537bd2a16aed96de1f71a46a41a7c5b8f26b420449f6": 30, "/sha256/eca9a32bbbb8a92d786b5cb7021d9be4de3ea4bf374a865a9a3af9dd6c2b3ee7": 30, "/sha256/ecaf2f7b3f3354db0d41e8104606d273c67020d0a2f44515071653fac341ecce": 30, "/sha256/ecaa682ed6ed7d4a0f1a3d64dee027b81ddc07af7d35b3bf4422f875d4af8302": 30, "/sha256/ecaeec74e719cb10c8ec2b6bddea9dbd637b294da235c7482746e20a76a0cf19": 30}
    identifier = "/sha256/eca8d73d099dfebcbf3e20a57935101839ac2c7739ccc9f2dd36d77fb8b54212"

    with TemporaryDirectory() as working_dir:
        storage = Storage(working_dir)
        pass1 = storage.like(identifier, external)
        pass2 = storage.like(identifier)
        assert pass1 == pass2, f"{pass1} vs {pass2}"
        old_max = libernet.disk.MAX_LIKE
        libernet.disk.MAX_LIKE = 5
        pass3 = storage.like(identifier)
        assert len(pass3) == 5
        libernet.disk.MAX_LIKE = old_max
        assert storage.get(identifier) == None

        try:
            value = storage[identifier]
            assert False, value

        except KeyError:
            pass


if __name__ == "__main__":
    test_basics()
    test_corners()
