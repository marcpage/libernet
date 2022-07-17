#!/usr/bin/env python3

""" Libernet settings
"""

import json
import os

import libernet.tools.encrypt


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
