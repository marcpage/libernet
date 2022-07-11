#!/usr/bin/env python3

import Crypto.Cipher.AES
import Crypto.PublicKey.RSA
import Crypto.Cipher.PKCS1_OAEP
import Crypto.Signature.PKCS1_v1_5

import libernet.tools.hash

def aes_encrypt(key, data, iv=b'0'*Crypto.Cipher.AES.block_size):
    cipher = Crypto.Cipher.AES.new(key, Crypto.Cipher.AES.MODE_CBC, iv)
    padding_length = 16 - (len(data) % 16)
    padded = data + bytes([padding_length]) * padding_length
    return cipher.encrypt(padded)


def aes_decrypt(key, data, iv=b'0'*Crypto.Cipher.AES.block_size):
    cipher = Crypto.Cipher.AES.new(key, Crypto.Cipher.AES.MODE_CBC, iv)
    padded = cipher.decrypt(data)
    return padded[:-padded[-1]]


class RSA_Identity:
    @staticmethod
    def create(bits):
        return RSA_Identity(key=Crypto.PublicKey.RSA.generate(bits))

    def __init__(self, description=None, key=None):
        if key:
            self.__private = key
            self.__public = key.public_key()
            private_pem = key.export_key('PEM')
            public_pem = self.__public.export_key('PEM')
            identifier = libernet.tools.hash.sha256_data_identifier(public_pem)
            self.__description = {
                'private': private_pem.decode('utf-8'),
                'public': public_pem.decode('utf-8'),
                'identifier': identifier
            }

        if description:
            self.__description = description
            self.__public = Crypto.PublicKey.RSA.import_key(description['public'])
            private = description.get('private', None)


            if private:
                self.__private = Crypto.PublicKey.RSA.import_key(private)
            else:
                self.__private = None

    def identifier(self):
        return self.__description['identifier']

    def private_description(self):
        return self.__description

    def public_description(self):
        return {'public': self.__description['public'], 'identifier': self.__description['identifier']}

    def encrypt(self, data):
        public_cipher = Crypto.Cipher.PKCS1_OAEP.new(self.__public)
        return public_cipher.encrypt(data)

    def decrypt(self, data):
        private_cipher = Crypto.Cipher.PKCS1_OAEP.new(self.__private)
        return private_cipher.decrypt(data)

    def sign(self, hasher):
        signer = Crypto.Signature.PKCS1_v1_5.new(self.__private)
        return signer.sign(hasher)

    def verify(self, hasher, signature):
        validater = Crypto.Signature.PKCS1_v1_5.new(self.__public)
        return validater.verify(hasher, signature)
