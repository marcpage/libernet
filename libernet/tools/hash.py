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


def identifier_to_bits(identifier):
    as_bytes = binary_from_identifier(identifier)
    as_binary = [bin(b).lstrip('0b').zfill(8) for b in as_bytes]
    return ''.join(as_binary)


def identifier_match_score(id1, id2):
    """ 20 bits match takes 10 seconds (21 bits is 3 characters 28 bits is 4)
        29,246 hashes/second
         18 154,630:1 (5,904) 0.1 minutes
         19 304,007:1 (3,003) 0.2 minutes
         20 578,906:1 (1,577) 0.3 minutes
         21 1,046,944:1 (872) 0.6 minutes 3 characters
         22 1,352,497:1 (675) 0.8 minutes
         23 5,499,614:1 (166) 3.1 minutes
         24 6,762,488:1 (135) 3.9 minutes
         25 14,724,773:1 (62) 8.4 minutes
         26 26,851,057:1 (34) 15.3 minutes
         27 30,431,198:1 (30) 17.3 minutes
         28 152,155,994:1 (6) 1.4 hours     4 characters
         29 228,233,992:1 (4) 2.2 hours
         30 304,311,989:1 (3) 2.9 hours
         31 456,467,984:1 (2) 4.3 hours
         33 912,935,968:1 (1) 8.7 hours
    """
    bits1 = identifier_to_bits(id1)
    bits2 = identifier_to_bits(id2)

    for index, bit1_2 in enumerate(zip(bits1,bits2)):
        if bit1_2[0] != bit1_2[1]:
            return index

    return min(len(bits1), len(bits2))
