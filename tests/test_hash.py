#!/usr/bin/env python3

import struct

import libernet.tools.hash

def test_identifier_match_score():
    id1 = libernet.tools.hash.sha256_data_identifier(b'test')

    for i in range(200000, 300000):
        d2 = struct.pack('q', i)
        id2 = libernet.tools.hash.sha256_data_identifier(d2)
        score = libernet.tools.hash.identifier_match_score(id1, id2)
        assert score < 6 or id1[0] == id2[0]
        assert score < 12 or id2[1] == id2[1]
        assert score < 18 or id2[3] == id2[3]

    score = libernet.tools.hash.identifier_match_score(id1, id1)
    assert score == 256

