#!/usr/bin/env python3


import libernet.block


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

if __name__ == "__main__":
    test_basic()
