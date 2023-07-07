#!/usr/bin/env python3

""" Connection to a remote Libernet server
    Messages:
        {
            'command': ['shutdown' | 'request' | 'response'],
            'url': ['/sha256/{identifier}' | '/sha256/like/{identifier}'],
            'node': '{server:port}',
            'fulfilled': [True | False],
            'result': bytes|list,
        }
        shutdown - handled
        find - handled
        found - sent
        request - handled
        received - sent
        sent - sent
"""


import threading

import libernet.proxy


# commands
SHUTDOWN = 'shutdown'
REQUEST = 'request'
RESPONSE = 'response'
HANDLED = [SHUTDOWN, REQUEST, RESPONSE]

# keys
COMMAND = 'command'
URL = 'url'
NODE = 'node'
FULFILLED = 'fulfilled'
RESULT = 'result'


class Remote(threading.Thread):

    def __init__(self, server:str, port:int, message_center, args):
        self.__node = f"{server}:{port}"
        self.__args = args
        self.__proxy = libernet.proxy.Storage(server, port)
        self.__outgoing = message_center
        self.__incoming = message_center.new_channel()
        self.__requests = []
        threading.Thread.__init__(self)
        self.daemon = True
        self.start()

    def __fetch_message(self, block=False) -> dict:
        """Get a message from the queue and return None if not blocking"""
        try:
            return self.__incoming.get(block=block)
        except queue.Empty:
            return None

    def __fetch_messages(self) -> [dict]:
        """Get all pending messages in the queue"""
        first_message = self.__fetch_message(block=True)
        messages = [first_message]

        while True:
            message = self.__fetch_message()

            if message is None:
                break

            messages.append(message)

        return messages

    def __is_shutdown(self, message):
        if message[COMMAND] == SHUTDOWN:
            self.__proxy.shutdown()
            return True

        return False

    def __is_request(self, message):
        if message[COMMAND] == REQUEST:
            fulfilled = message[FULFILLED]
            from_me = message[NODE] == self.__node
            already_seen = message[URL] in self.__requests

            if not fulfilled and not from_me and not already_seen:
                self.__requests.append(message[URL])
            elif fulfilled and already_seen:
                self.__requests.remove(message[URL])

            return True

        return False

    def __is_response(self, message):
        if message[COMMAND] == RESPONSE:
            if message[URL] in self.__requests:
                self.__requests.remove(message[URL])

            return True

        return False

    def __update_state(self):
        for message in self.__fetch_messages():
            if message[COMMAND] not in HANDLED:  # ignore other messages
                continue

            if self.__is_shutdown(message):
                return False

            if self.__is_request(message) or self.__is_response(message):
                pass


        return True

    def run(self):
        while self.__update_state():
            pass

