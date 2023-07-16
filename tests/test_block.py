#!/usr/bin/env python3


import libernet.block

from libernet.hash import sha256_data_identifier, identifier_match_score
from libernet.url import address_of


def test_basic():
    storage = {}
    test_cases = ['', '😀', 'testing', 'e'*1000, 'testing'*1000]

    for data in test_cases:
        url, _ = libernet.block.store(data.encode('utf-8'), storage)
        duplicate = libernet.block.fetch(url, storage)
        assert duplicate.decode('utf-8') == data, f"{duplicate} vs {data.encode('utf-8')}"

        url, _ = libernet.block.store(data.encode('utf-8'), storage, encrypt=False)
        duplicate = libernet.block.fetch(url, storage)
        assert duplicate.decode('utf-8') == data, f"{duplicate} vs {data.encode('utf-8')}"


def test_padding():
    storage = {}
    test_cases = ['', '😀', 'testing', 'e'*1000, 'testing'*1000]
    similar = sha256_data_identifier('match me'.encode('utf-8'))

    for data in test_cases:
        url, _ = libernet.block.store(data.encode('utf-8'), storage, similar=similar, score=12)
        duplicate = libernet.block.fetch(url, storage, was_similar=True)
        assert duplicate == data.encode('utf-8'), f"{duplicate} vs {data.encode('utf-8')}"
        assert identifier_match_score(url.split('/')[2], similar) >= 12, (url.split('/')[2], similar)

        url, _ = libernet.block.store(data.encode('utf-8'), storage, encrypt=False, similar=similar, score=12)
        duplicate = libernet.block.fetch(url, storage, was_similar=True)
        assert duplicate == data.encode('utf-8'), f"{duplicate} vs {data.encode('utf-8')}"
        assert identifier_match_score(url.split('/')[2], similar) >= 12, (url.split('/')[2], similar)


def test_password():
    storage = {}
    test_cases = ['', '😀', 'testing', 'e'*1000, 'testing'*1000]
    similar = sha256_data_identifier('match me'.encode('utf-8'))
    password = "Setec Astronomy"

    for data in test_cases:
        url, _ = libernet.block.store(data.encode('utf-8'), storage, encrypt=password)
        duplicate = libernet.block.fetch(url, storage, was_similar=False)
        assert duplicate == data.encode('utf-8'), f"{duplicate} vs {data.encode('utf-8')}"

        url, _ = libernet.block.store(data.encode('utf-8'), storage, encrypt=password, similar=similar, score=12)
        duplicate = libernet.block.fetch(address_of(url), storage, was_similar=True, password=password)
        assert duplicate == data.encode('utf-8'), f"{duplicate} vs {data.encode('utf-8')}"
        assert identifier_match_score(url.split('/')[2], similar) >= 12, (url.split('/')[2], similar)


if __name__ == "__main__":
    test_basic()
    test_padding()
    test_password()
