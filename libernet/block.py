#!/usr/bin/env python3


""" Managing blocks of data and their address
"""


import zlib
from random import randbytes

from libernet.encrypt import aes_encrypt, aes_decrypt
from libernet.hash import sha256_data_identifier, binary_from_identifier
from libernet.hash import identifier_match_score


MAX_BLOCK_SIZE = 1024 * 1024


def address(url: str) -> str:
    """extract just the address of the block"""
    parts = url.split("/")
    assert len(parts) >= 3, parts
    assert parts[1] == "sha256", parts
    return f"/sha256/{parts[2]}"


def __padding_suffixes(similar: str, encrypt, score: int) -> str:
    """determine random suffixes for address matching
    If we are matching, we will need to add random data to try and match.
    If we are encrypting, add the suffix to the encrypted data.
    If we're not encrypting, add the suffix to the data (before compression).
    """
    if not similar:
        return b"", b""

    suffix = b"\x00" + randbytes(score // 8 + 1).replace(b"\x00", b"")
    data_suffix = b"" if encrypt else suffix
    encrypted_suffix = suffix if encrypt else b""
    return (data_suffix, encrypted_suffix)


def __maybe_compress(data: bytes, encrypt) -> bytes:
    """Data is compressed if:
    1. We will have a hash of the original data in the url (no password encrypt)
    2. The compressed data is not bigger than the original data
    """
    if encrypt and encrypt is not True:
        return data

    compressed = zlib.compress(data, 9)
    return compressed if len(compressed) <= len(data) else data


def __key_and_kind(data: bytes, encrypt) -> (str, str):
    """If we are encrypting ...
    And we're using the contents hash as a key: return hash key
    Or we're using a password hash as a key: return hash password
    """
    if not encrypt:
        return None, None

    if encrypt is True:
        return sha256_data_identifier(data), "aes256"

    return sha256_data_identifier(encrypt.encode("utf-8")), "password"


def __maybe_encrypt(
    data: bytes, key_identifier: str, kind: str, padded: str, end_suffix: str
) -> (bytes, str, str):
    """If we're encrypting:
    1. Create the binary key to encrypt with
    2. Encrypt the data
    3. Get the identifier of the encrypted data
    """
    if key_identifier is None:
        data_identifier = sha256_data_identifier(padded)
        return data, f"/sha256/{data_identifier}", data_identifier

    key_value = binary_from_identifier(key_identifier)
    encrypted = aes_encrypt(key_value, data) + end_suffix
    encrypted_identifier = sha256_data_identifier(encrypted)
    return (
        encrypted,
        f"/sha256/{encrypted_identifier}/{kind}/{key_identifier}",
        encrypted_identifier,
    )


def store(data: bytes, storage, encrypt=True, similar=None, score=12) -> (str, bytes):
    """prepares data, stores it, and returns url and processed data
    data - raw data to store
    storage - a dict-like object
    encrypt - should the data be encrypted with the sha256 has being the aes256 key
            If it is a string, it is a password to use for encryption
                Note: if password is specified, the block cannot be decompressed
                        We do not have the contents hash to know if it was compressed
    similar - An identifier to make this similar to (padding to match)
    score - The number of bits that need to match the similar identifier
    """
    assert len(data) <= MAX_BLOCK_SIZE, f"{len(data)} > {MAX_BLOCK_SIZE}: {data}"

    while True:
        start_suffix, end_suffix = __padding_suffixes(similar, encrypt, score)
        padded = data + start_suffix
        compressed = __maybe_compress(padded, encrypt)
        key, kind = __key_and_kind(padded, encrypt)
        block, url, ident = __maybe_encrypt(compressed, key, kind, padded, end_suffix)

        if not similar or identifier_match_score(similar, ident) >= score:
            break

    assert len(block) <= MAX_BLOCK_SIZE, f"{len(block)} > {MAX_BLOCK_SIZE}: {block}"
    storage[address(url)] = block
    return url, block


def __maybe_unpad(data: bytes, was_similar: bool) -> bytes:
    """Determine if the black needs to have padding removed
    data - the data that may have padding
    was_similar - is padding expected
    """
    if was_similar:
        assert b"\x00" in data, data
        return data[: data.rfind(b"\x00")]

    return data


def __maybe_decrypt(parts: [str], data: bytes, was_similar: bool) -> bytes:
    """determine if the contents are encrypted and decrypt them
    parts - the parts of the url
    data - raw data block
    was_similar - was the data saved to a known location?
    """
    if len(parts) == 3:
        return data

    assert parts[3] in ["password", "aes256"], parts
    unpadded = __maybe_unpad(data, was_similar)
    block_contents = aes_decrypt(binary_from_identifier(parts[4]), unpadded)
    return block_contents


def __maybe_uncompress(parts: [str], data: bytes, was_similar: bool) -> (bytes, str):
    """determine if the contents are compressed and uncompress them
    parts - the parts of the url
    data - the data to uncompress (not encrypted)
    was_similar - was the data saved to a known location?
    """
    encrypted = len(parts) == 5

    if was_similar and not encrypted:
        data_identifier = sha256_data_identifier(data)

        if data_identifier == parts[-1]:
            return __maybe_unpad(data, was_similar), sha256_data_identifier(data)

    if encrypted and parts[-2] == "password":
        return data, sha256_data_identifier(data)

    if sha256_data_identifier(data) == parts[-1]:
        return data, sha256_data_identifier(data)

    uncompressed = zlib.decompress(data)
    assert sha256_data_identifier(uncompressed) == parts[-1]
    unpadded = uncompressed if encrypted else __maybe_unpad(uncompressed, was_similar)
    return unpadded, sha256_data_identifier(uncompressed)


def unpack(url: str, data: bytes, was_similar=False) -> bytes:
    """convert a block with a given url to original data
    url - the url used to fetch the data, may contain aes256 component
    data - the stored block of data, if None, return None
    was_similar - was the address matched to a similar identifier
    """
    if data is None:
        return None

    parts = url.split("/")
    assert len(parts) in [3, 5], parts
    assert parts[0] == "", parts
    assert parts[1] == "sha256", parts
    decrypted = __maybe_decrypt(parts, data, was_similar)
    uncompressed, identifier = __maybe_uncompress(parts, decrypted, was_similar)
    assert parts[-2] == "password" or identifier == parts[-1]
    return uncompressed


def fetch(url: str, storage, was_similar=False) -> bytes:
    """request a block from storage"""
    return unpack(url, storage.get(address(url)), was_similar)
