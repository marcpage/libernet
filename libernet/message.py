#!/usr/bin/env python3

""" Libernet message management.
    Create a message.Center()
    Have the message.Center create a Channel
    Sending messages broadcasts that message to every channel
    Each channel will receive its own copy of the message
"""


import threading
import queue
import logging


LOGGING_TIMEOUT_SECONDS = 0.500


class Center(threading.Thread):
    """global multi-threaded messaging system"""

    def __init__(self):
        """create queues and start thread"""
        self.__input = queue.Queue()
        self.__output = []
        self.__running = True
        self.__lock = threading.Lock()
        threading.Thread.__init__(self)
        self.daemon = True
        self.start()

    def __broadcast(self, message):
        """Send a message to all recipients"""
        with self.__lock:
            out_queues = list(self.__output)

        for out in out_queues:
            out.put(message)

    def run(self):
        while self.active():
            message = self.__input.get()

            if message is None:
                continue

            self.__broadcast(message)

        self.__broadcast(None)

    def shutdown(self):
        """no more messages will be sent"""
        self.__running = False
        self.__input.put(None)

    def active(self):
        """Are we still processing"""
        if self.__running:
            return True

        return self.__input.qsize() > 0

    def new_channel(self):
        """Creates a new channel to receive messages"""
        channel = queue.Queue()

        with self.__lock:
            self.__output.append(channel)

        return channel

    def close_channel(self, channel: queue.Queue):
        """prevent future message coming from message center"""
        with self.__lock:
            if channel in self.__output:
                self.__output.remove(channel)

    def send(self, message):
        """Send a message to all recipients"""
        assert self.__running, "Message center shutdown, cannot send message"
        assert message is not None
        self.__input.put(message)


class Logger(threading.Thread):
    """Logs all messages from a message center"""

    def __init__(self, messages: Center):
        self.__messages = messages
        threading.Thread.__init__(self)
        self.daemon = True
        self.start()

    def run(self):
        channel = self.__messages.new_channel()

        while self.__messages.active():
            try:
                message = channel.get(timeout=LOGGING_TIMEOUT_SECONDS)
                logging.info("Message received: %s", message)

            except queue.Empty:
                pass

        self.__messages.close_channel(channel)
