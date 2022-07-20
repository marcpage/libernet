#!/usr/bin/env python3

""" Libernet encryption
"""

import base64

import Crypto.Cipher.AES
import Crypto.PublicKey.RSA
import Crypto.Cipher.PKCS1_OAEP
import Crypto.Signature.PKCS1_v1_5

import libernet.tools.hash


# pylint: disable=C0103
def aes_encrypt(key, data, iv=b"0" * Crypto.Cipher.AES.block_size):
    """encrypt"""
    cipher = Crypto.Cipher.AES.new(key, Crypto.Cipher.AES.MODE_CBC, iv)
    padding_length = 16 - (len(data) % 16)
    padded = data + bytes([padding_length]) * padding_length
    return cipher.encrypt(padded)


# pylint: disable=C0103
def aes_decrypt(key, data, iv=b"0" * Crypto.Cipher.AES.block_size):
    """decrypt"""
    cipher = Crypto.Cipher.AES.new(key, Crypto.Cipher.AES.MODE_CBC, iv)
    padded = cipher.decrypt(data)
    return padded[: -padded[-1]]


class RSA_Identity:
    """RSA Identity"""

    @staticmethod
    def create(bits):
        """create RSA Identity with given bits"""
        return RSA_Identity(key=Crypto.PublicKey.RSA.generate(bits))

    def __init__(self, description=None, key=None):
        """create RSA Identity from either description or a constructed key"""
        if key:
            self.__private = key
            self.__public = key.public_key()
            private_pem = key.export_key("PEM")
            public_pem = self.__public.export_key("PEM")
            identifier = libernet.tools.hash.sha256_data_identifier(public_pem)
            self.__description = {
                "private": private_pem.decode("utf-8"),
                "public": public_pem.decode("utf-8"),
                "identifier": identifier,
            }

        if description:
            self.__description = description
            self.__public = Crypto.PublicKey.RSA.import_key(description["public"])
            private = description.get("private", None)
            self.__private = (
                Crypto.PublicKey.RSA.import_key(private) if private else None
            )

    def identifier(self):
        """Get the identifier for the identity"""
        return self.__description["identifier"]

    def private_description(self):
        """Get the private description"""
        return self.__description

    def public_description(self):
        """Get the public description"""
        return {
            "public": self.__description["public"],
            "identifier": self.__description["identifier"],
        }

    def encrypt(self, data):
        """Use the public key to encrypt so only the private key can decrypt"""
        public_cipher = Crypto.Cipher.PKCS1_OAEP.new(self.__public)
        return public_cipher.encrypt(data)

    def decrypt(self, data):
        """Use the private key to decrypt"""
        private_cipher = Crypto.Cipher.PKCS1_OAEP.new(self.__private)
        return private_cipher.decrypt(data)

    def sign(self, hasher):
        """Use the private key to sign the data"""
        signer = Crypto.Signature.PKCS1_v1_5.new(self.__private)
        return base64.b64encode(signer.sign(hasher)).decode("ascii")

    def sign_utf8(self, text):
        """sign text"""
        return self.sign(libernet.tools.hash.sha256_hasher(text.encode("utf-8")))

    def verify(self, hasher, signature):
        """use the public key to verify the signature of the data"""
        validater = Crypto.Signature.PKCS1_v1_5.new(self.__public)
        # pylint: disable=E1102
        return validater.verify(hasher, base64.b64decode(signature))

    def verify_utf8(self, text, signature):
        """verify signature of text"""
        return self.verify(
            libernet.tools.hash.sha256_hasher(text.encode("utf-8")), signature
        )
