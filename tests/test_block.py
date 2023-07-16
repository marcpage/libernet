#!/usr/bin/env python3


import libernet.block
from libernet.hash import sha256_data_identifier, identifier_match_score
from libernet.block import address, parse_identifier


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
        duplicate = libernet.block.fetch(address(url), storage, was_similar=True, password=password)
        assert duplicate == data.encode('utf-8'), f"{duplicate} vs {data.encode('utf-8')}"
        assert identifier_match_score(url.split('/')[2], similar) >= 12, (url.split('/')[2], similar)


def test_parse_identifier():
    identifier, key, contents = parse_identifier("/sha256/1ab47a1928b8a229e6100c33a6568b589bb9a3f5d604a71324f7c4ddd022be86/aes256/1ac2e73f2292a62b3a11fc45c9a37264517d10afbf2f074a482a9b1c499a49a5")
    assert identifier == "1ab47a1928b8a229e6100c33a6568b589bb9a3f5d604a71324f7c4ddd022be86", identifier
    assert key == "1ac2e73f2292a62b3a11fc45c9a37264517d10afbf2f074a482a9b1c499a49a5", key
    assert contents == "1ac2e73f2292a62b3a11fc45c9a37264517d10afbf2f074a482a9b1c499a49a5", contents

    identifier, key, contents = parse_identifier("/sha256/1ab47a1928b8a229e6100c33a6568b589bb9a3f5d604a71324f7c4ddd022be86/like/1ac2e73f2292a62b3a11fc45c9a37264517d10afbf2f074a482a9b1c499a49a5")
    assert identifier == "1ab47a1928b8a229e6100c33a6568b589bb9a3f5d604a71324f7c4ddd022be86", identifier
    assert key == "1ac2e73f2292a62b3a11fc45c9a37264517d10afbf2f074a482a9b1c499a49a5", key
    assert contents is None, contents

    identifier, key, contents = parse_identifier("/sha256/1ab47a1928b8a229e6100c33a6568b589bb9a3f5d604a71324f7c4ddd022be86")
    assert identifier == "1ab47a1928b8a229e6100c33a6568b589bb9a3f5d604a71324f7c4ddd022be86", identifier
    assert key is None, key
    assert contents == "1ab47a1928b8a229e6100c33a6568b589bb9a3f5d604a71324f7c4ddd022be86", contents


if __name__ == "__main__":
    test_basic()
    test_padding()
    test_password()
    test_parse_identifier()
