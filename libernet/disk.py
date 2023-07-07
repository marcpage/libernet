#!/usr/bin/env python3

""" Manages disk operations
"""


import os
import json

from libernet.hash import identifier_match_score, IDENTIFIER_SIZE


GROUP_NIBBLES = 3
MAX_LIKE = 100


class Storage:
    """data storage on disk as a dict-like object"""

    def __init__(self, path):
        self.__path = os.path.join(path, "data")

    def __dir_of(self, identifier):
        directory = os.path.join(self.__path, identifier[:GROUP_NIBBLES])
        return directory

    def __path_of(self, identifier):
        return os.path.join(self.__dir_of(identifier), identifier[GROUP_NIBBLES:])

    @staticmethod
    def __parse_identifier(url):
        parts = url.split("/")
        assert len(parts) in [3, 4]
        assert parts[0] == ""
        assert parts[1] == "sha256"
        similar = len(parts) == 4 and parts[0] == "like"
        assert len(parts) == 3 or similar
        identifier = parts[-1]
        assert len(identifier) == IDENTIFIER_SIZE
        return identifier

    @staticmethod
    def __like_file_path(identifier: str, data_dir: str):
        return os.path.join(data_dir, identifier[GROUP_NIBBLES:] + ".like.json")

    def __load_like_cache(self, like_path: str):
        if not os.path.isfile(like_path):
            return {}

        with open(like_path, "r", encoding="utf-8") as like_file:
            return json.load(like_file)

    def __save_like_cache(self, identifier: str, like_path: str, *likes: dict):
        os.makedirs(os.path.split(like_path)[0], exist_ok=True)
        merged = {k: v for l in likes for k, v in l.items()}
        top = list(merged)
        top.sort(
            key=lambda u: identifier_match_score(
                Storage.__parse_identifier(u), identifier
            ),
            reverse=True,
        )

        for url in top[MAX_LIKE:]:
            del merged[url]  # remove everything after the top

        with open(like_path, "w", encoding="utf-8") as like_file:
            json.dump(merged, like_file)

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
        assert not key.startswith("/sha256/like/")
        identifier = Storage.__parse_identifier(key)
        path = self.__path_of(identifier)
        os.makedirs(self.__dir_of(identifier), exist_ok=True)

        with open(path, "wb") as block_file:
            block_file.write(value)

    def get(self, key: str, default: bytes = None) -> bytes:
        """Get the data for a given path"""
        assert not key.startswith("/sha256/like/")
        path = self.__path_of(Storage.__parse_identifier(key))

        if not os.path.isfile(path):
            return default

        with open(path, "rb") as block_file:
            return block_file.read()

    def like(self, key: str, initial: dict = None) -> dict:
        """Gets list of identifiers that best match this one
        initial - values to add to the cache
        returns dictionary of urls to size of data on disk
        """
        initial = {} if initial is None else initial
        identifier = Storage.__parse_identifier(key)
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
        return os.path.isfile(self.__path_of(Storage.__parse_identifier(key)))
