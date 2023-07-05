#!/usr/bin/env python3


""" Managing blocks of data and their address
"""


import zlib

from libernet.encrypt import aes_encrypt, aes_decrypt
from libernet.hash import sha256_data_identifier, binary_from_identifier


MAX_BLOCK_SIZE = 1024 * 1024


def address(url: str) -> str:
    """extract just the address of the block"""
    parts = url.split("/")
    assert len(parts) >= 3, parts
    assert parts[1] == "sha256", parts
    return f"/sha256/{parts[2]}"


def store(data: bytes, storage, encrypt=True) -> (str, bytes):
    """prepares data, stores it, and returns url and processed data
    data - raw data to store
    storage - a dict-like object
    encrypt - should the data be encrypted with the sha256 has being the aes256 key
    """
    assert len(data) <= MAX_BLOCK_SIZE, f"{len(data)}: {data}"
    compressed = zlib.compress(data, 9)
    contents_identifier = sha256_data_identifier(data)

    if len(data) < len(compressed):
        compressed = data

    if encrypt:
        key = binary_from_identifier(contents_identifier)
        block_contents = aes_encrypt(key, compressed)
        identifier = sha256_data_identifier(block_contents)
        url = f"/sha256/{identifier}/aes256/{contents_identifier}"
    else:
        block_contents = compressed
        url = f"/sha256/{contents_identifier}"

    storage[address(url)] = block_contents
    return url, block_contents


def unpack(url: str, data: bytes) -> bytes:
    """convert a block with a given url to original data
    url - the url used to fetch the data, may contain aes256 component
    data - the stored block of data
    """
    if data is None:
        return None

    assert len(data) <= MAX_BLOCK_SIZE, f"{len(data)}: {data}"
    parts = url.split("/")
    encrypted = len(parts) == 5
    block_hash = parts[2]
    data_hash = parts[4] if encrypted else parts[2]
    assert len(parts) == 3 or encrypted, parts
    assert not encrypted or block_hash == sha256_data_identifier(data)

    if encrypted:
        data = aes_decrypt(binary_from_identifier(data_hash), data)

    if sha256_data_identifier(data) == data_hash:
        return data  # not compressed

    data = zlib.decompress(data)
    assert sha256_data_identifier(data) == data_hash
    return data


def fetch(url: str, storage) -> bytes:
    """request a block from storage"""
    return unpack(url, storage.get(address(url)))
