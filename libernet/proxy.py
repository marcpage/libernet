#!/usr/bin/env python3

""" Acts as a block storage but uses a Libernet server as the storage
"""


import threading
import queue
import logging
import json

import requests


class Storage(threading.Thread):
    """Proxy storage class to remote server"""

    def __init__(self, server: str, port: int):
        self.__base_url = f"http://{server}:{port}"
        self.__running = True
        self.__session = requests.Session()
        self.__session_lock = threading.Lock()
        self.__input = queue.Queue()
        self.__event = threading.Event()
        threading.Thread.__init__(self)
        self.daemon = False  # make sure we can send all data before shutting down
        self.start()

    def __setitem__(self, key: str, item: bytes):
        """Queues data to be sent"""
        assert self.__running, "Proxy has been shutdown()"
        self.__input.put((key, item))
        self.__event.clear()

    def get(self, key, default=None) -> bytes:
        """Waits for all sent items to be flushed then requests data"""
        assert self.__running, "Proxy has been shutdown()"
        self.__event.wait()  # wait for all sent items to be flushed

        with self.__session_lock:
            response = self.__session.get(self.__base_url + key)

        if response.status_code != 200:
            return default

        return response.content

    def like(self, key: str) -> list:
        """gets a list of keys that are best-matches to given key"""
        assert self.__running, "Proxy has been shutdown()"
        self.__event.wait()  # wait for all sent items to be flushed
        parts = key.split('/')
        assert parts[0] == ''
        assert parts[1] == 'sha256'

        with self.__session_lock:
            response = self.__session.get(f"{self.__base_url}/{parts[1]}/like/{parts[2]}")

        if response.status_code != 200:
            return {}

        return json.loads(response.content.decode('utf-8'))

    def __getitem__(self, key: str) -> bytes:
        """Just calls get() to get data from server, after send queue is flushed"""
        assert self.__running, "Proxy has been shutdown()"
        result = self.get(key)

        if result is None:
            raise KeyError(f"{key} not found on {self.__base_url}")

        return result

    def __contains__(self, key: str) -> bool:
        """Does the block exist in the server"""
        assert self.__running, "Proxy has been shutdown()"
        self.__event.wait()  # wait for all sent items to be flushed

        with self.__session_lock:
            response = self.__session.head(self.__base_url + key)

        return response.status_code == 200

    def __fetch_message(self, block=False) -> (bytes, str):
        """Get a message from the queue and return None if not blocking"""
        try:
            return self.__input.get(block=block)
        except queue.Empty:
            return None

    def __fetch_messages(self) -> [(bytes, str)]:
        """Get all pending messages in the queue"""
        self.__event.set()
        first_message = self.__fetch_message(block=True)
        self.__event.clear()
        messages = [first_message]

        while True:
            message = self.__fetch_message()

            if message is None:
                break

            messages.append(message)

        return messages

    def active(self) -> bool:
        """Are we still processing"""
        if self.__running:
            return True

        return self.__input.qsize() > 0

    def shutdown(self):
        """no more messages will be sent"""
        self.__running = False
        self.__input.put(None)

    def run(self):
        """The queued data-send thread"""
        while self.active():
            messages = self.__fetch_messages()

            for message in messages:
                if message is None:
                    continue

                logging.info(
                    "Sending %d bytes of data to %s",
                    len(message[1]),
                    self.__base_url + message[0],
                )

                with self.__session_lock:
                    response = self.__session.put(
                        self.__base_url + message[0], data=message[1]
                    )

                if response.status_code != 200:
                    logging.warning(
                        "Sending %d bytes of data to %s -> %d: %s",
                        len(message[1]),
                        self.__base_url + message[0],
                        response.status_code,
                        response.content,
                    )
