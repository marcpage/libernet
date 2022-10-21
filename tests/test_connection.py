#!/usr/bin/env python3

import tempfile
import multiprocessing
import os
import time
import sys
import signal

import libernet.plat.network
import libernet.server
import libernet.tools.connection
import libernet.tools.settings
import libernet.tools.message

def __asked_to_quit(*args):
    print("Exiting gracefully")
    sys.exit(0)


def __run_server(port:int, storage:str, debug:bool, key_size:int, fake_remote:bool):
    """ This test code needs to be here to be able to set the local machine test value """
    args = type('',(),{})
    args.port = port
    args.storage = storage
    args.debug = debug
    testing = libernet.plat.network.TEST_NOT_LOCAL_MACHINE
    libernet.plat.network.TEST_NOT_LOCAL_MACHINE = fake_remote
    signal.signal(signal.SIGINT, __asked_to_quit)
    signal.signal(signal.SIGTERM, __asked_to_quit)
    libernet.server.handle_args(args, key_size=key_size)
    libernet.plat.network.TEST_NOT_LOCAL_MACHINE = testing


def test_remote_identity():
    port_to_use = 8086
    key_size = 1024
    debug = False
    fake_remote = False
    messages = libernet.tools.message.Center()

    with tempfile.TemporaryDirectory() as server_storage, tempfile.TemporaryDirectory() as client_storage:
        server = multiprocessing.Process(
            target=__run_server,
            args=(port_to_use, server_storage, debug, key_size, fake_remote),
            daemon=True,
        )
        server.start()
        print("Waiting for server to come up")
        time.sleep(1.0)
        print("Starting up client")
        client_settings = libernet.tools.settings.App(client_storage, key_size)
        print(f"client: {client_settings.identity().identifier()}")
        connection = libernet.tools.connection.Connection('localhost', port_to_use, messages, client_settings)
        print("Waiting for connection to warm up")
        time.sleep(0.200)
        print("Sending messages")
        for i in range(0, 100):
            messages.send({'test': i})
        with open(os.path.join(server_storage, "log.txt"), 'r') as log_file:
            print(log_file.read())
        print(f"remote: {connection.identity().identifier()}")
        time.sleep(0.200)
        assert connection.identity() is not None
        server.terminate()
        server.join()

if __name__ == "__main__":
    test_remote_identity()
