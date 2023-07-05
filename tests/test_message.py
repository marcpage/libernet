#!/usr/bin/env python3


import threading
import time

from libernet.message import Center


THREADS = 100
MESSAGES = 100

def __communicate(message_center:Center, thread:int):
    channel = message_center.new_channel()

    for i in range(0, MESSAGES):
        message_center.send(f"{thread}:{i}")
        time.sleep(0.000_001)

    while True:
        message = channel.get()

        if message is None:
            break


def test_high_threading():
    message_center = Center()
    main_channel = message_center.new_channel()

    threads = [threading.Thread(target=__communicate, args=(message_center,t), daemon=False)
                for t in range(0, THREADS)]

    for thread in threads:
        thread.start()

    high = 0

    while True:
        if main_channel.qsize() > high:
            high = main_channel.qsize()
            time.sleep(0.100)
        else:
            break

    message_center.shutdown()

    for thread in threads:
        thread.join()

    messages = []
    while True:
        message = main_channel.get()

        if message is None:
            break

        messages.append(message)

    assert len(messages) == THREADS * MESSAGES
    assert len(set(messages)) == len(messages)

    try:
        message_center.send("failure")
        raise SyntaxError("We should have thrown an exception")

    except AssertionError:
        pass


if __name__ == "__main__":
    test_high_threading()
