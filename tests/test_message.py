#!/usr/bin/env python3

import threading

import libernet.tools.message

def __validate(message_center):
    last_value = None

    while message_center.active():
        this_value = message_center.receive()

        if this_value is not None:
            if last_value is not None:
                assert this_value > last_value
                assert this_value - last_value == 1

            last_value = this_value

    assert last_value is not None
    assert last_value == 9999, f"Last value was {last_value}"


def __send_numbers(message_center):
    for i in range(0, 10000):
        message_center.send(i)

    message_center.shutdown()


def test_high_threading():
    message_center = libernet.tools.message.Center()
    sender_thread = threading.Thread(target=__send_numbers, args=(message_center,), daemon=False)
    sender_thread.start()
    threads = [threading.Thread(target=__validate, args=(message_center,), daemon=False)
                for _ in range(0, 100)]

    for thread in threads:
        thread.start()

    sender_thread.join()

    try:
        message_center.send("failure")
        assert False, "We should have thrown an exception"

    except AssertionError:
        pass

    for thread in threads:
        thread.join()
