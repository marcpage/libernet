#!/usr/bin/env python3

""" Manages disk operations
"""


import os
import json

from libernet.hash import IDENTIFIER_SIZE


GROUP_NIBBLES = 3
MAX_LIKE = 100


class Storage:
    def __init__(self, path):
        self.__path = os.path.join(path, 'data')

    @staticmethod
    def __dir_of(self, identifier):
        directory = os.path.join(self.__path, identifier[:GROUP_NIBBLES])
        return directory

    @staticmethod
    def __path_of(self, identifier):
        return os.path.join(Storage.__dir_of(identifier), identifier[GROUP_NIBBLES:])

    @staticmethod
    def __parse_identifier(self, url):
        parts = url.split("/")
        assert len(parts) in [3, 4]
        assert parts[0] == ''
        assert parts[1] == 'sha256'
        similar = len(parts) == 4 and parts[0] == "like"
        assert len(parts) == 3 or similar
        identifier = parts[-1]
        assert len(identifier) == IDENTIFIER_SIZE
        return identifier

    @staticmethod
    def __like_file_path(self, identifier:str, data_dir:str):
        return os.path.join(data_dir, identifier[GROUP_NIBBLES:] + '.like.json')

    def __load_like_cache(self, like_file:str):
        if not os.path.isfile(like_file):
            return {}

        with open(like_file, 'r') as like_file:
            return json.load(like_file)

    def __save_like_cache(self, identifier:str, like_file:str, *likes:dict):
        os.makedirs(os.path.split(like_file)[0], exist_ok=True)
        merged = {k:v for l in likes for k,v in l.items()}
        top = list(merged)
        top.sort(
            key=lambda u:identifier_match_score(Storage.__parse_identifier(u), identifier),
            reverse=True
        )

        for url in top[MAX_LIKE:]:
            del merged[url]  # remove everything after the top

        with open(like_file, 'w') as like_file:
            json.dump(merged, like_file)

    def __find_like_files(self, identifier:str, data_dir:str):
        if not os.path.isdir(data_dir):
            return {}

        potential = [
            identifier[:GROUP_NIBBLES] + n
            for n in os.listdir(data_dir)
            if len(n) == IDENTIFIER_SIZE - GROUP_NIBBLES
        ]
        return {f"/sha256/{i}": os.path.getsize(Storage.__path_of(i))
                for i in potential}

    def __setitem__(self, key: str, value: bytes):
        assert not key.startswith('/sha256/like/')
        identifier = Storage.__parse_identifier(key)
        path = Storage.__path_of(identifier)
        os.makedirs(Storage.__dir_of(identifier), exist_ok=True)

        with open(path, "wb") as block_file:
            block_file.write(value)

    def get(self, key:str, default:bytes=None) -> bytes:
        assert not key.startswith('/sha256/like/')
        path = Storage.__path_of(key)

        if not os.path.isfile(path):
            return None

        with open(path, 'rb') as block_file:
            return block_file.read()

    def like(self, key: str, initial:dict=None) -> dict:
        initial = {} if initial is None else initial
        identifier = Storage.__parse_identifier(key)
        data_dir = Storage.__dir_of(identifier)
        like_path = Storage.__like_file_path(identifier, data_dir)
        cached = self.__load_like_cache(like_file)
        return self.__save_like_cache(identifier, like_file, initial, cached)



    def __getitem__(self, key: str) -> bytes:
        """Just calls get() to get data from server, after send queue is flushed"""
        result = self.get(key)

        if result is None:
            raise KeyError(f"{key} not found in {self.__path}")

        return result

    def __contains__(self, key: str) -> bool:
        return os.path.isfile(Storage.__path_of(Storage.parse_identifier(key)))
