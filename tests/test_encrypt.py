#!/usr/bin/env python3

import json

import libernet.encrypt
import libernet.hash


def test_RSA_Identity():
    identity = libernet.encrypt.RsaIdentity.create(1024)
    assert len(identity.identifier()) == 64
    serialized_private = json.dumps(identity.private_description())
    restored_identity = libernet.encrypt.RsaIdentity(
        json.loads(serialized_private)
    )
    serialized_public = json.dumps(identity.public_description())
    restored_public = libernet.encrypt.RsaIdentity(json.loads(serialized_public))
    encrypted = restored_public.encrypt(b"to encrypt")
    assert restored_identity.decrypt(encrypted) == b"to encrypt"
    assert identity.decrypt(encrypted) == b"to encrypt"
    signature = identity.sign_utf8("to sign")
    assert signature == restored_identity.sign_utf8("to sign")
    assert restored_public.verify_utf8("to sign", signature)
    assert not restored_public.verify_utf8("not signed", signature)


def test_aes():
    key_text = libernet.hash.sha256_data_identifier(b"password")
    key = libernet.hash.binary_from_identifier(key_text)
    text = "hello world".encode("utf-8")
    value = libernet.encrypt.aes_encrypt(key, text)
    result = libernet.encrypt.aes_decrypt(key, value)
    assert result == text
