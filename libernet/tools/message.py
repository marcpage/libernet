#!/usr/bin/env python3

""" Libernet message management
"""

import threading
import queue


class Center(threading.Thread):

    def __init__(self):
        self.__input = queue.Queue()
        self.__output = {}
        self.__running = True
        self.__lock = threading.Lock()
        threading.Thread.__init__(self)
        self.daemon= True
        self.start()

    def __broadcast(self, message):
        with self.__lock:
            out_queues = [self.__output[i]['queue'] for i in self.__output]

        for out in out_queues:
            out.put(message)

    def run(self):
        while self.active():
            message = self.__input.get()
            self.__broadcast(message)

        self.__broadcast(None)

    def shutdown(self):
        self.__running = False

    def active(self):
        return self.__running or self.__input.qsize() > 0

    def send(self, message):
        assert self.__running, "Message center shutdown, cannot send message"
        self.__input.put(message)

    def receive(self):
        """ Get the next broadcast message
            if the return value is None and message_center.active() == False,
                there will be no more messages
        """
        thread_id = threading.current_thread().ident

        with self.__lock:
            self.__output[thread_id] = self.__output.get(thread_id, dict())
            thread_info = self.__output[thread_id]

            if 'queue' not in thread_info:
                thread_info['queue'] = queue.Queue()

        return thread_info['queue'].get()



