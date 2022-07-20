#!/usr/bin/env python3

""" Libernet settings
"""

import json
import os

import libernet.tools.encrypt
import libernet.tools.block
import libernet.plat.timestamp

HTTP_TIMESTAMP = "X-Libernet-Timestamp"
HTTP_PATH = "X-Libernet-Path"
HTTP_AUTHOR = "X-Libernet-Author"
HTTP_SIGNATURE = "X-Libernet-Signature"


class App:
    """Settings for a libernet server app"""

    def __init__(self, storage_dir, key_size=4096):
        """create an app settings"""
        self.__dir = storage_dir
        self.__settings_path = os.path.join(storage_dir, "settings.json")

        try:
            with open(self.__settings_path, "r", encoding="utf-8") as settings_file:
                self.__settings = json.load(settings_file)

        except FileNotFoundError:
            self.__settings = {}

        try:
            self.__identity = libernet.tools.encrypt.RSA_Identity(
                self.__settings["identity"]
            )

        except KeyError:
            self.__identity = libernet.tools.encrypt.RSA_Identity.create(key_size)
            self.__settings["identity"] = self.__identity.private_description()
            libernet.tools.block.store_block(
                self.__identity.public_description()["public"].encode("utf-8"),
                self.__dir,
                encrypt=False,
            )

        self.flush()

    def flush(self):
        """flush any changes to the settings to disk"""
        with open(self.__settings_path, "w", encoding="utf-8") as settings_file:
            json.dump(self.__settings, settings_file)

    def storage(self):
        """Get the storage directory"""
        return self.__dir

    def identity(self):
        """Get the private identity for this server"""
        return self.__identity

    def sign_request(self, path):
        """Sign a request for a path"""
        headers = {
            HTTP_TIMESTAMP: f"{libernet.plat.timestamp.create():0.3f}",
            HTTP_PATH: path,
            HTTP_AUTHOR: self.__identity.identifier(),
        }
        headers[HTTP_SIGNATURE] = self.__identity.sign_utf8(
            f"{headers[HTTP_TIMESTAMP]}:{headers[HTTP_PATH]}"
        )
        return headers


def verify_request(headers, storage):
    """Verify request signature"""
    timestamp = headers.get(HTTP_TIMESTAMP, None)
    path = headers.get(HTTP_PATH, None)
    author = headers.get(HTTP_AUTHOR, None)
    signature = headers.get(HTTP_SIGNATURE, None)

    if timestamp is None or path is None:
        return False

    if author is None or signature is None:
        return False

    author_description = {
        "identifier": author,
        "public": libernet.tools.block.retrieve(f"/sha256/{author}", storage),
    }
    author_identity = libernet.tools.encrypt.RSA_Identity(author_description)

    return author_identity.verify_utf8(f"{timestamp}:{path}", signature)
