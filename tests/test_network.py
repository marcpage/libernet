#!/usr/bin/env python3

import libernet.plat.network

def test_is_on_machine():
    assert libernet.plat.network.machine_name("127.0.0.1") == "localhost"
    assert libernet.plat.network.is_on_machine("127.0.0.1")
