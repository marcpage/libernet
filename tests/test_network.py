#!/usr/bin/env python3

import libernet.plat.network

def test_is_on_machine():
    local_machine = libernet.plat.network.TEST_NOT_LOCAL_MACHINE
    
    libernet.plat.network.TEST_NOT_LOCAL_MACHINE = True
    assert libernet.plat.network.machine_name("127.0.0.1") == "localhost"
    assert not libernet.plat.network.is_on_machine("127.0.0.1")

    libernet.plat.network.TEST_NOT_LOCAL_MACHINE = False
    assert libernet.plat.network.machine_name("127.0.0.1") == "localhost"
    assert libernet.plat.network.is_on_machine("127.0.0.1")
    
    libernet.plat.network.TEST_NOT_LOCAL_MACHINE = local_machine
