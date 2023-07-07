#!/usr/bin/env python3

import struct

import libernet.hash


def test_identifier_match_score():
    """ Median time to get match score:
        16 - 1 second
        20 - 10 seconds
        24 - 5 minutes
    """
    id1 = libernet.hash.sha256_data_identifier(b"test")

    for i in range(200000, 300000):
        d2 = struct.pack("q", i)
        id2 = libernet.hash.sha256_data_identifier(d2)
        score = libernet.hash.identifier_match_score(id1, id2)
        assert score < 6 or id1[0] == id2[0]
        assert score < 12 or id2[1] == id2[1]
        assert score < 18 or id2[3] == id2[3]

    score = libernet.hash.identifier_match_score(id1, id1)
    assert score == 256


def test_binary_from_identifier_first_nibble_zero():
    old_validate = libernet.hash.VALIDATE_IDENTIFIER_SIZE
    libernet.hash.VALIDATE_IDENTIFIER_SIZE = False
    libernet.hash.binary_from_identifier('16019dc1e2ba3d35e462012d4481cc165dd0a77a095f9123ffeeab0f5a5ad98')
    libernet.hash.VALIDATE_IDENTIFIER_SIZE = old_validate

def test_identifier_match_score_first_nibble_zero():
    old_validate = libernet.hash.VALIDATE_IDENTIFIER_SIZE
    libernet.hash.VALIDATE_IDENTIFIER_SIZE = False
    assert libernet.hash.identifier_match_score('f', 'f0') == 0
    libernet.hash.VALIDATE_IDENTIFIER_SIZE = old_validate


def test_identifier_match_score_first_byte_zero():
    old_validate = libernet.hash.VALIDATE_IDENTIFIER_SIZE
    libernet.hash.VALIDATE_IDENTIFIER_SIZE = False
    assert libernet.hash.identifier_match_score('ff', 'ff00') == 0
    libernet.hash.VALIDATE_IDENTIFIER_SIZE = old_validate


if __name__ == "__main__":
    test_identifier_match_score()
    test_binary_from_identifier_first_nibble_zero()
    test_identifier_match_score_first_nibble_zero()
    test_identifier_match_score_first_byte_zero()
