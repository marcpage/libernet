#!/usr/bin/env python3

""" Manages disk operations
"""


import os
import json
import random
import threading

import libernet.url

from libernet.hash import identifier_match_score, IDENTIFIER_SIZE
from libernet.url import LIKE


GROUP_NIBBLES = 3
MAX_LIKE = 100


class Storage:
    """data storage on disk as a dict-like object"""

    def __init__(self, path):
        self.__path = os.path.join(path, "data")
        self.__lock = threading.Lock()

    def __dir_of(self, identifier):
        directory = os.path.join(self.__path, identifier[:GROUP_NIBBLES])
        return directory

    def __path_of(self, identifier):
        return os.path.join(self.__dir_of(identifier), identifier[GROUP_NIBBLES:])

    def __safe_save(self, path: str, data, binary: bool):
        """returns the contents of a temp file and then moves the temp file into place
        if not binary it is assumed to be json
        """
        mode = "wb" if binary else "w"
        encoding = None if binary else "utf-8"
        basename, extension = os.path.splitext(path)

        with self.__lock:
            while True:
                temp_path = (
                    basename + f"_{random.randrange(0xffffffff):08x}" + extension
                )

                if not os.path.exists(temp_path):
                    break

            with open(temp_path, mode, encoding=encoding) as data_file:
                if binary:
                    data_file.write(data)
                else:
                    json.dump(data, data_file)

            if os.path.isfile(path):
                os.remove(path)

            os.rename(temp_path, path)

    def __read_file(self, path: str, binary: bool):
        """returns the contents of a file
        if not binary it is assumed to be json
        """
        mode = "rb" if binary else "r"
        encoding = None if binary else "utf-8"

        with self.__lock:
            if not os.path.isfile(path):
                return None

            with open(path, mode, encoding=encoding) as data_file:
                if binary:
                    return data_file.read()

                return json.load(data_file)

    @staticmethod
    def __like_file_path(identifier: str, data_dir: str):
        return os.path.join(data_dir, identifier[GROUP_NIBBLES:] + ".like.json")

    def __load_like_cache(self, like_path: str):
        contents = self.__read_file(like_path, binary=False)
        return {} if contents is None else contents

    def __save_like_cache(self, identifier: str, like_path: str, *likes: dict):
        os.makedirs(os.path.split(like_path)[0], exist_ok=True)
        merged = {k: v for l in likes for k, v in l.items()}
        top = list(merged)
        top.sort(
            key=lambda u: identifier_match_score(libernet.url.parse(u)[0], identifier),
            reverse=True,
        )

        for url in top[MAX_LIKE:]:
            del merged[url]  # remove everything after the top

        self.__safe_save(like_path, merged, binary=False)
        return merged

    def __find_like_files(self, identifier: str, data_dir: str):
        if not os.path.isdir(data_dir):
            return {}

        potential = [
            identifier[:GROUP_NIBBLES] + n
            for n in os.listdir(data_dir)
            if len(n) == IDENTIFIER_SIZE - GROUP_NIBBLES
        ]
        return {f"/sha256/{i}": os.path.getsize(self.__path_of(i)) for i in potential}

    def __setitem__(self, key: str, value: bytes):
        identifier, _, _, kind = libernet.url.parse(key)
        assert len(identifier) == IDENTIFIER_SIZE, f"{len(identifier)} {identifier}"
        assert kind != LIKE
        identifier = libernet.url.parse(key)[0]
        path = self.__path_of(identifier)

        # we must overwrite every time because previous copy may be corrupt
        os.makedirs(self.__dir_of(identifier), exist_ok=True)
        self.__safe_save(path, value, binary=True)

    def get(self, key: str, default: bytes = None) -> bytes:
        """Get the data for a given path"""
        identifier, _, _, _ = libernet.url.parse(key)
        assert len(identifier) == IDENTIFIER_SIZE, f"{len(identifier)} {identifier}"
        path = self.__path_of(identifier)
        contents = self.__read_file(path, binary=True)
        return default if contents is None else contents

    def like(self, key: str, initial: dict = None) -> dict:
        """Gets list of identifiers that best match this one
        initial - values to add to the cache
        returns dictionary of urls to size of data on disk
        """
        initial = {} if initial is None else initial
        identifier = libernet.url.parse(key)[0]
        data_dir = self.__dir_of(identifier)
        like_path = Storage.__like_file_path(identifier, data_dir)
        cached = self.__load_like_cache(like_path)
        on_disk = self.__find_like_files(identifier, data_dir)
        return self.__save_like_cache(identifier, like_path, initial, cached, on_disk)

    def __getitem__(self, key: str) -> bytes:
        """Just calls get() to get data from server, after send queue is flushed"""
        result = self.get(key)

        if result is None:
            raise KeyError(f"{key} not found in {self.__path}")

        return result

    def __contains__(self, key: str) -> bool:
        return os.path.isfile(self.__path_of(libernet.url.parse(key)[0]))
