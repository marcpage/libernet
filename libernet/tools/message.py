#!/usr/bin/env python3

""" Libernet message management.
    Create a message.Center() and pass it around 
    Sending messages broadcasts that message t every recipient
    Each Python thread will receive its own copy of the message
    Each recevier should be on its own thread
"""

import threading
import queue


class Center(threading.Thread):
    """global multi-threaded messaging system"""

    def __init__(self):
        """create queues and start thread"""
        self.__input = queue.Queue()
        self.__output = {}
        self.__running = True
        self.__lock = threading.Lock()
        threading.Thread.__init__(self)
        self.daemon = True
        self.start()

    def __broadcast(self, message):
        """Send a message to all recipients"""
        with self.__lock:
            out_queues = [i[1]["queue"] for i in self.__output.items()]

        for out in out_queues:
            out.put(message)

    def run(self):
        """the main thread"""
        while self.active():
            message = self.__input.get()
            self.__broadcast(message)

        self.__broadcast(None)

    def shutdown(self):
        """no more messages will be sent"""
        self.__running = False

    def active(self):
        """Are we still processing"""
        if self.__running:
            return True

        if self.__input.qsize() > 0:
            return True

        thread_id = threading.current_thread().ident

        with self.__lock:
            return (
                thread_id in self.__output
                and "queue" in self.__output[thread_id]
                and self.__output[thread_id]["queue"].qsize() > 0
            )

    def send(self, message):
        """Send a message to all recipients"""
        assert self.__running, "Message center shutdown, cannot send message"
        self.__input.put(message)

    def receive(self):
        """Get the next broadcast message
        if the return value is None and message_center.active() == False,
            there will be no more messages
        """
        thread_id = threading.current_thread().ident

        with self.__lock:
            self.__output[thread_id] = self.__output.get(thread_id, {})
            self.__output[thread_id]["id"] = self.__output[thread_id].get(
                "id", thread_id
            )
            assert self.__output[thread_id]["id"] == thread_id
            thread_info = self.__output[thread_id]

            if "queue" not in thread_info:
                thread_info["queue"] = queue.Queue()

        return thread_info["queue"].get()
