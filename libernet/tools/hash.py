#!/usr/bin/env python3

import Crypto.Hash.SHA256
import base64


def sha256_hasher(data):
    return Crypto.Hash.SHA256.new(data)


def sha256_data_identifier(data):
    hashed = sha256_hasher(data).digest()
    return base64.urlsafe_b64encode(hashed).decode('ascii')


def binary_from_identifier(identifier):
    return base64.urlsafe_b64decode(identifier)


"""
def sha256_str(string):
    return Crypto.Hash.SHA256.new(s.encode('utf-8')).digest()
"""
