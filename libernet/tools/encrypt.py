#!/usr/bin/env python3

import Crypto.Cipher.AES


def aes_encrypt(key, data, iv=b'0'*Crypto.Cipher.AES.block_size):
    cipher = Crypto.Cipher.AES.new(key, Crypto.Cipher.AES.MODE_CBC, iv)
    padding_length = 16 - (len(data) % 16)
    padded = data + bytes([padding_length]) * padding_length
    return cipher.encrypt(padded)


def aes_decrypt(key, data, iv=b'0'*Crypto.Cipher.AES.block_size):
    cipher = Crypto.Cipher.AES.new(key, Crypto.Cipher.AES.MODE_CBC, iv)
    padded = cipher.decrypt(data)
    return padded[:-padded[-1]]


def RSA_Identity:
    @staticmethod
    def create(bits):
        return RSA(key=Crypto.PublicKey.RSA.generate(bits))

    def __init__(self, description=None, key=None):
        if key:
            self.__private = key
            self.__private_pem = key.export_key('PEM')
            self.__public = key.public_key()
            self.__public_pem = self.__public.export_key('PEM')



def rsa_create(bits):
    return Crypto.PublicKey.RSA.generate(bits)
