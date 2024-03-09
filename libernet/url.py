#!/usr/bin/env python3

""" common url helpers
"""


from libernet.hash import IDENTIFIER_SIZE


# URL parts
SHA256 = "sha256"
AES256 = "aes256"
PASSWORD = "passphrase"
LIKE = "like"  # /sha256/*like*/{identifier}
ENCRYPTION_TYPES = [AES256, PASSWORD]


def parse(url: str) -> (str, str, str, str):
    """Gets the data block id, encryption key, contents hash, and kind of url
    /sha256/{hash}
    /sha256/like/{hash}
    /sha256/{hash}/aes256/{hash}
    /sha256/{hash}/password/{hash}
    """
    parts = url.split("/")
    assert len(parts) >= 3, parts
    assert parts[0] == "", parts
    assert parts[1] == SHA256, parts
    is_like = parts[2] == LIKE
    is_just_data = len(parts) == 3
    is_encrypted = len(parts) == 5 and parts[3] in (PASSWORD, AES256)
    is_contents_encrypted = is_encrypted and parts[3] == AES256
    assert is_just_data or is_like or is_encrypted
    address = parts[3] if is_like else parts[2]
    assert len(address) == IDENTIFIER_SIZE, f"{len(address)}: {address}"
    key = parts[4] if is_encrypted else None
    contents = key if is_contents_encrypted else None if is_encrypted else address
    kind = (
        SHA256
        if is_just_data
        else LIKE if is_like else AES256 if is_contents_encrypted else PASSWORD
    )
    return (address, key, contents, kind)


def for_data_block(identifier: str, like=False) -> str:
    """Gets a URL from a block identifier"""
    assert len(identifier) == IDENTIFIER_SIZE, f"{len(identifier)}: {identifier}"
    return f"/{SHA256}/{LIKE}/{identifier}" if like else f"/{SHA256}/{identifier}"


def for_encrypted(identifier: str, key: str, key_type: str = AES256) -> str:
    """Gets a URL for an encrypted data block"""
    assert len(key) == IDENTIFIER_SIZE, f"{len(key)}: {key}"
    assert key_type in ENCRYPTION_TYPES, key_type
    return f"{for_data_block(identifier)}/{key_type}/{key}"


def address_of(url: str) -> str:
    """extract just the address of the block"""
    address, _, _, _ = parse(url)
    return for_data_block(address)
