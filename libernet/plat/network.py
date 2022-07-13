#!/usr/bin/env python3

""" Libernet platform specific network tools
"""

import ipaddress
import socket


def machine_name(remote_addr):
    """Get the dns name of remote_addr"""
    return socket.getnameinfo((remote_addr, 0), 0)[0]


def is_on_machine(remote_addr):
    """determines if the remote_addr represents the machine serving up the content"""
    # 127.0.0.1     private local loopback     non-link-local
    # 192.168.86.29 private local non-loopback non-link-local
    # 192.168.86.41 private local non-loopback non-link-local
    address = ipaddress.ip_address(remote_addr)
    local_address = address.is_private and not address.is_global
    name_matches = address.is_loopback or socket.getfqdn() == machine_name(remote_addr)
    return local_address and name_matches
