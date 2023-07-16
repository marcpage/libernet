#!/usr/bin/env python3


import time
import pprint
from types import SimpleNamespace
from random import randbytes

import libernet.block
import libernet.backup
import libernet.disk
from libernet.backup import ENV_USER, ENV_PASS, KEY_SERVICE, KEY_USER
from libernet.hash import sha256_data_identifier


class Keyring:
    def __init__(self):
        self.data = {}

    def get_password(self, n1:str, n2:str) -> str:
        return self.data.get(n1, {}).get(n2, None)

    def set_password(self, n1:str, n2:str, p:str):
        self.data[n1] = self.data.get(n1, {})
        self.data[n1][n2] = p


class Store:
    def __init__(self):
        self.data = {}
        self.active = True

    @staticmethod
    def __like(url1, url2) -> bool:
        is_like = url1.split('/')[-1][:3] == url2.split('/')[-1][:3]
        return is_like

    def __setitem__(self, key: str, value: bytes):
        self.data[key] = value

    def get(self, key: str, default: bytes = None) -> bytes:
        return self.data.get(key, default)

    def like(self, key: str) -> dict:
        found = {k:len(self.data[k]) for k in self.data if Store.__like(k, key)}
        return found

    def __getitem__(self, key: str) -> bytes:
        found = self.data[key]
        return found

    def __contains__(self, key: str) -> bool:
        return key in self.data

    def active(self) -> bool:
        return self.active

    def shutdown(self):
        self.active = False

    def __str__(self):
        return str(self.data)


def test_arg_parser():
    parser = libernet.backup.get_arg_parser()
    assert parser is not None


def test_arg_processor():
    user_input = libernet.backup.USER_INPUT
    pass_input = libernet.backup.PASSWORD_INPUT
    libernet.backup.PASSWORD_INPUT = lambda _:'bad password'
    libernet.backup.USER_INPUT = lambda _:'bad user'
    environment={}
    keychain = Keyring()

    args = libernet.backup.process_args(SimpleNamespace(action='add', months=12, user='John', passphrase='Setec Astronomy', machine='localhost', source=['libernet'], yes=True, destination=None, no=False, keychain=False, environment=False), environment=environment, key=keychain)
    assert args.user == 'John'
    assert args.passphrase == 'Setec Astronomy'
    args = libernet.backup.process_args(SimpleNamespace(action='list', months=12, user='John', passphrase='Setec Astronomy', machine='localhost', source=[], yes=True, destination=None, no=False, keychain=False, environment=False), environment=environment, key=keychain)
    assert args.user == 'John'
    assert args.passphrase == 'Setec Astronomy'
    args = libernet.backup.process_args(SimpleNamespace(action='remove', months=12, user='John', passphrase='Setec Astronomy', machine='localhost', source=['libernet'], yes=True, destination=None, no=False, keychain=False, environment=False), environment=environment, key=keychain)
    assert args.user == 'John'
    assert args.passphrase == 'Setec Astronomy'
    args = libernet.backup.process_args(SimpleNamespace(action='backup', months=12, user='John', passphrase='Setec Astronomy', machine='localhost', source=[], yes=True, destination=None, no=False, keychain=False, environment=False), environment=environment, key=keychain)
    assert args.user == 'John'
    assert args.passphrase == 'Setec Astronomy'
    args = libernet.backup.process_args(SimpleNamespace(action='restore', months=12, user='John', passphrase='Setec Astronomy', machine='localhost', source=[], yes=True, destination=None, no=False, keychain=False, environment=False), environment=environment, key=keychain)
    assert args.user == 'John'
    assert args.passphrase == 'Setec Astronomy'

    keychain = Keyring()
    environment[ENV_USER] = 'John'
    args = libernet.backup.process_args(SimpleNamespace(action='add', months=12, user=None, passphrase='Setec Astronomy', machine='localhost', source=['libernet'], yes=True, destination=None, no=False, keychain=False, environment=True), environment=environment, key=keychain)
    assert args.user == 'John'
    assert args.passphrase == 'Setec Astronomy'
    args = libernet.backup.process_args(SimpleNamespace(action='list', months=12, user=None, passphrase='Setec Astronomy', machine='localhost', source=[], yes=True, destination=None, no=False, keychain=False, environment=True), environment=environment, key=keychain)
    assert args.user == 'John'
    assert args.passphrase == 'Setec Astronomy'
    args = libernet.backup.process_args(SimpleNamespace(action='remove', months=12, user=None, passphrase='Setec Astronomy', machine='localhost', source=['libernet'], yes=True, destination=None, no=False, keychain=False, environment=True), environment=environment, key=keychain)
    assert args.user == 'John'
    assert args.passphrase == 'Setec Astronomy'
    args = libernet.backup.process_args(SimpleNamespace(action='backup', months=12, user=None, passphrase='Setec Astronomy', machine='localhost', source=[], yes=True, destination=None, no=False, keychain=False, environment=True), environment=environment, key=keychain)
    assert args.user == 'John'
    assert args.passphrase == 'Setec Astronomy'
    args = libernet.backup.process_args(SimpleNamespace(action='restore', months=12, user=None, passphrase='Setec Astronomy', machine='localhost', source=[], yes=True, destination=None, no=False, keychain=False, environment=True), environment=environment, key=keychain)
    assert args.user == 'John'
    assert args.passphrase == 'Setec Astronomy'

    keychain = Keyring()
    environment[ENV_PASS] = 'Setec Astronomy'
    args = libernet.backup.process_args(SimpleNamespace(action='add', months=12, user=None, passphrase=None, machine='localhost', source=['libernet'], yes=True, destination=None, no=False, keychain=False, environment=True), environment=environment, key=keychain)
    assert args.user == 'John'
    assert args.passphrase == 'Setec Astronomy'
    args = libernet.backup.process_args(SimpleNamespace(action='list', months=12, user=None, passphrase=None, machine='localhost', source=[], yes=True, destination=None, no=False, keychain=False, environment=True), environment=environment, key=keychain)
    assert args.user == 'John'
    assert args.passphrase == 'Setec Astronomy'
    args = libernet.backup.process_args(SimpleNamespace(action='remove', months=12, user=None, passphrase=None, machine='localhost', source=['libernet'], yes=True, destination=None, no=False, keychain=False, environment=True), environment=environment, key=keychain)
    assert args.user == 'John'
    assert args.passphrase == 'Setec Astronomy'
    args = libernet.backup.process_args(SimpleNamespace(action='backup', months=12, user=None, passphrase=None, machine='localhost', source=[], yes=True, destination=None, no=False, keychain=False, environment=True), environment=environment, key=keychain)
    assert args.user == 'John'
    assert args.passphrase == 'Setec Astronomy'
    args = libernet.backup.process_args(SimpleNamespace(action='restore', months=12, user=None, passphrase=None, machine='localhost', source=[], yes=True, destination=None, no=False, keychain=False, environment=True), environment=environment, key=keychain)
    assert args.user == 'John'
    assert args.passphrase == 'Setec Astronomy'

    keychain = Keyring()
    del environment[ENV_USER]
    args = libernet.backup.process_args(SimpleNamespace(action='add', months=12, user='John', passphrase=None, machine='localhost', source=['libernet'], yes=True, destination=None, no=False, keychain=False, environment=True), environment=environment, key=keychain)
    assert args.user == 'John'
    assert args.passphrase == 'Setec Astronomy'
    args = libernet.backup.process_args(SimpleNamespace(action='list', months=12, user='John', passphrase=None, machine='localhost', source=[], yes=True, destination=None, no=False, keychain=False, environment=True), environment=environment, key=keychain)
    assert args.user == 'John'
    assert args.passphrase == 'Setec Astronomy'
    args = libernet.backup.process_args(SimpleNamespace(action='remove', months=12, user='John', passphrase=None, machine='localhost', source=['libernet'], yes=True, destination=None, no=False, keychain=False, environment=True), environment=environment, key=keychain)
    assert args.user == 'John'
    assert args.passphrase == 'Setec Astronomy'
    args = libernet.backup.process_args(SimpleNamespace(action='backup', months=12, user='John', passphrase=None, machine='localhost', source=[], yes=True, destination=None, no=False, keychain=False, environment=True), environment=environment, key=keychain)
    assert args.user == 'John'
    assert args.passphrase == 'Setec Astronomy'
    args = libernet.backup.process_args(SimpleNamespace(action='restore', months=12, user='John', passphrase=None, machine='localhost', source=[], yes=True, destination=None, no=False, keychain=False, environment=True), environment=environment, key=keychain)
    assert args.user == 'John'
    assert args.passphrase == 'Setec Astronomy'

    keychain = Keyring()
    del environment[ENV_PASS]
    libernet.backup.USER_INPUT = lambda _:'John'
    args = libernet.backup.process_args(SimpleNamespace(action='add', months=12, user=None, passphrase='Setec Astronomy', machine='localhost', source=['libernet'], yes=True, destination=None, no=False, keychain=True, environment=True), environment=environment, key=keychain)
    assert args.user == 'John'
    assert args.passphrase == 'Setec Astronomy'
    assert keychain.get_password(KEY_SERVICE, KEY_USER) == 'John', keychain.get_password(KEY_SERVICE, KEY_USER)
    assert keychain.get_password(KEY_SERVICE, KEY_USER+'_John') == 'Setec Astronomy'
    args = libernet.backup.process_args(SimpleNamespace(action='list', months=12, user=None, passphrase='Setec Astronomy', machine='localhost', source=[], yes=True, destination=None, no=False, keychain=True, environment=True), environment=environment, key=keychain)
    assert args.user == 'John'
    assert args.passphrase == 'Setec Astronomy'
    assert keychain.get_password(KEY_SERVICE, KEY_USER) == 'John'
    assert keychain.get_password(KEY_SERVICE, KEY_USER+'_John') == 'Setec Astronomy'
    args = libernet.backup.process_args(SimpleNamespace(action='remove', months=12, user=None, passphrase='Setec Astronomy', machine='localhost', source=['libernet'], yes=True, destination=None, no=False, keychain=True, environment=True), environment=environment, key=keychain)
    assert args.user == 'John'
    assert args.passphrase == 'Setec Astronomy'
    assert keychain.get_password(KEY_SERVICE, KEY_USER) == 'John'
    assert keychain.get_password(KEY_SERVICE, KEY_USER+'_John') == 'Setec Astronomy'
    args = libernet.backup.process_args(SimpleNamespace(action='backup', months=12, user=None, passphrase='Setec Astronomy', machine='localhost', source=[], yes=True, destination=None, no=False, keychain=True, environment=True), environment=environment, key=keychain)
    assert args.user == 'John'
    assert args.passphrase == 'Setec Astronomy'
    assert keychain.get_password(KEY_SERVICE, KEY_USER) == 'John'
    assert keychain.get_password(KEY_SERVICE, KEY_USER+'_John') == 'Setec Astronomy'
    args = libernet.backup.process_args(SimpleNamespace(action='restore', months=12, user=None, passphrase='Setec Astronomy', machine='localhost', source=[], yes=True, destination=None, no=False, keychain=True, environment=True), environment=environment, key=keychain)
    assert args.user == 'John'
    assert args.passphrase == 'Setec Astronomy'
    assert keychain.get_password(KEY_SERVICE, KEY_USER) == 'John'
    assert keychain.get_password(KEY_SERVICE, KEY_USER+'_John') == 'Setec Astronomy'

    keychain = Keyring()
    libernet.backup.PASSWORD_INPUT = lambda _:'Setec Astronomy'
    args = libernet.backup.process_args(SimpleNamespace(action='add', months=12, user=None, passphrase=None, machine='localhost', source=['libernet'], yes=True, destination=None, no=False, keychain=True, environment=True), environment=environment, key=keychain)
    assert args.user == 'John'
    assert args.passphrase == 'Setec Astronomy'
    assert keychain.get_password(KEY_SERVICE, KEY_USER) == 'John'
    assert keychain.get_password(KEY_SERVICE, KEY_USER+'_John') == 'Setec Astronomy'
    args = libernet.backup.process_args(SimpleNamespace(action='list', months=12, user=None, passphrase=None, machine='localhost', source=[], yes=True, destination=None, no=False, keychain=True, environment=True), environment=environment, key=keychain)
    assert args.user == 'John'
    assert args.passphrase == 'Setec Astronomy'
    assert keychain.get_password(KEY_SERVICE, KEY_USER) == 'John'
    assert keychain.get_password(KEY_SERVICE, KEY_USER+'_John') == 'Setec Astronomy'
    args = libernet.backup.process_args(SimpleNamespace(action='remove', months=12, user=None, passphrase=None, machine='localhost', source=['libernet'], yes=True, destination=None, no=False, keychain=True, environment=True), environment=environment, key=keychain)
    assert args.user == 'John'
    assert args.passphrase == 'Setec Astronomy'
    assert keychain.get_password(KEY_SERVICE, KEY_USER) == 'John'
    assert keychain.get_password(KEY_SERVICE, KEY_USER+'_John') == 'Setec Astronomy'
    args = libernet.backup.process_args(SimpleNamespace(action='backup', months=12, user=None, passphrase=None, machine='localhost', source=[], yes=True, destination=None, no=False, keychain=True, environment=True), environment=environment, key=keychain)
    assert args.user == 'John'
    assert args.passphrase == 'Setec Astronomy'
    assert keychain.get_password(KEY_SERVICE, KEY_USER) == 'John'
    assert keychain.get_password(KEY_SERVICE, KEY_USER+'_John') == 'Setec Astronomy'
    args = libernet.backup.process_args(SimpleNamespace(action='restore', months=12, user=None, passphrase=None, machine='localhost', source=[], yes=True, destination=None, no=False, keychain=True, environment=True), environment=environment, key=keychain)
    assert args.user == 'John'
    assert args.passphrase == 'Setec Astronomy'
    assert keychain.get_password(KEY_SERVICE, KEY_USER) == 'John'
    assert keychain.get_password(KEY_SERVICE, KEY_USER+'_John') == 'Setec Astronomy'

    keychain = Keyring()
    libernet.backup.USER_INPUT = lambda _:'bad user'
    args = libernet.backup.process_args(SimpleNamespace(action='add', months=12, user='John', passphrase=None, machine='localhost', source=['libernet'], yes=True, destination=None, no=False, keychain=True, environment=True), environment=environment, key=keychain)
    assert args.user == 'John'
    assert args.passphrase == 'Setec Astronomy'
    assert keychain.get_password(KEY_SERVICE, KEY_USER) == 'John'
    assert keychain.get_password(KEY_SERVICE, KEY_USER+'_John') == 'Setec Astronomy'
    args = libernet.backup.process_args(SimpleNamespace(action='list', months=12, user='John', passphrase=None, machine='localhost', source=[], yes=True, destination=None, no=False, keychain=True, environment=True), environment=environment, key=keychain)
    assert args.user == 'John'
    assert args.passphrase == 'Setec Astronomy'
    assert keychain.get_password(KEY_SERVICE, KEY_USER) == 'John'
    assert keychain.get_password(KEY_SERVICE, KEY_USER+'_John') == 'Setec Astronomy'
    args = libernet.backup.process_args(SimpleNamespace(action='remove', months=12, user='John', passphrase=None, machine='localhost', source=['libernet'], yes=True, destination=None, no=False, keychain=True, environment=True), environment=environment, key=keychain)
    assert args.user == 'John'
    assert args.passphrase == 'Setec Astronomy'
    assert keychain.get_password(KEY_SERVICE, KEY_USER) == 'John'
    assert keychain.get_password(KEY_SERVICE, KEY_USER+'_John') == 'Setec Astronomy'
    args = libernet.backup.process_args(SimpleNamespace(action='backup', months=12, user='John', passphrase=None, machine='localhost', source=[], yes=True, destination=None, no=False, keychain=True, environment=True), environment=environment, key=keychain)
    assert args.user == 'John'
    assert args.passphrase == 'Setec Astronomy'
    assert keychain.get_password(KEY_SERVICE, KEY_USER) == 'John'
    assert keychain.get_password(KEY_SERVICE, KEY_USER+'_John') == 'Setec Astronomy'
    args = libernet.backup.process_args(SimpleNamespace(action='restore', months=12, user='John', passphrase=None, machine='localhost', source=[], yes=True, destination=None, no=False, keychain=True, environment=True), environment=environment, key=keychain)
    assert args.user == 'John'
    assert args.passphrase == 'Setec Astronomy'
    assert keychain.get_password(KEY_SERVICE, KEY_USER) == 'John'
    assert keychain.get_password(KEY_SERVICE, KEY_USER+'_John') == 'Setec Astronomy'

    libernet.backup.USER_INPUT = user_input
    libernet.backup.PASSWORD_INPUT = pass_input


def test_main():
    proxy = Store()
    other = Store()
    add_args = SimpleNamespace(months=12, user='John', passphrase='Setec Astronomy', machine='localhost', action='add', source=['libernet'], yes=True)
    print(f"{'='*40} adding first time {'='*40}")
    libernet.backup.main(add_args, proxy)
    print(f"{'='*40} adding second time {'='*40}")
    libernet.backup.main(add_args, proxy)
    print(f"{'='*40} done {'='*40}")
    libernet.backup.main(add_args, other)
    backup_args = SimpleNamespace(months=12, user='John', passphrase='Setec Astronomy', machine='localhost', action='backup', source=[], yes=True)
    print(f"{'='*40} backup B1 {'='*40}")
    libernet.backup.main(backup_args, other)
    print(f"{'='*40} backup A1 {'='*40}")
    libernet.backup.main(backup_args, proxy)
    print(f"{'='*40} backup B2 {'='*40}")
    libernet.backup.main(backup_args, other)
    print(f"{'='*40} backup A2 {'='*40}")
    libernet.backup.main(backup_args, proxy)
    print(f"{'='*40} merging {'='*40}")
    proxy.data.update(other.data)  # simulate backing up multiple machines and merging them
    print(f"{'='*40} backup A3 {'='*40}")
    libernet.backup.main(add_args, proxy)
    print(f"{'='*40} Done {'='*40}")


def test_random_data():
    test_data = [
        b'',
        b'e'*1000,
        b'{}',
        b'{"type": "backup"}',
        b'{"type": "backup", "timestamp": 0}',
        b'{"type": "backup", "timestamp": 0.0}',
        b'{"type": "backup", "timestamp": "0"}',
        b'{"type": "backup", "timestamp": 0, "user": "John"}',
        b'{"type": "backup", "timestamp": 0, "user": "John", "password": "Setec Astronomy"}',
    ]
    proxy = Store()
    add_args = SimpleNamespace(months=12, user='John', passphrase='Setec Astronomy', machine='localhost', action='add', source=['libernet'], yes=True)
    libernet.backup.main(add_args, proxy)
    backup_args = SimpleNamespace(months=12, user='John', passphrase='Setec Astronomy', machine='localhost', action='backup', source=[], yes=True)
    libernet.backup.main(backup_args, proxy)
    match_identifier = libernet.backup.get_similar_identifier(add_args)

    # make a "similar" block to every block (including the backup settings) with bad data
    for testset in test_data:
        print(f"testset = {testset}")
        libernet.block.store(testset, proxy, encrypt='Setec Astronomy', similar=match_identifier)
        libernet.block.store(testset, proxy, encrypt='bad password', similar=match_identifier)

    libernet.backup.main(add_args, proxy)


def test_add_remove():
    proxy = Store()
    add_args = SimpleNamespace(months=12, user='John', passphrase='Setec Astronomy', machine='localhost', action='add', source=['libernet'], yes=True)
    libernet.backup.main(add_args, proxy)
    libernet.backup.main(add_args, proxy)
    remove_args = SimpleNamespace(months=12, user='John', passphrase='Setec Astronomy', machine='localhost', action='remove', source=['libernet'], yes=True)
    libernet.backup.main(remove_args, proxy)
    libernet.backup.main(remove_args, proxy)
    libernet.backup.main(add_args, proxy)


def test_list():
    proxy = Store()
    add_args = SimpleNamespace(months=12, user='John', passphrase='Setec Astronomy', machine='localhost', action='add', source=['libernet'], yes=True)
    libernet.backup.main(add_args, proxy)
    backup_args = SimpleNamespace(months=12, user='John', passphrase='Setec Astronomy', machine='localhost', action='backup', source=[], yes=True)
    libernet.backup.main(backup_args, proxy)
    list_args = SimpleNamespace(months=12, user='John', passphrase='Setec Astronomy', machine='localhost', action='list', source=[], yes=True)
    libernet.backup.main(list_args, proxy)


def test_no():
    proxy = Store()
    add_args = SimpleNamespace(months=12, user='John', passphrase='Setec Astronomy', machine='localhost', action='add', source=['libernet'], yes=False, no=True)
    libernet.backup.main(add_args, proxy)


def test_input():
    user_input = libernet.backup.USER_INPUT

    libernet.backup.USER_INPUT = lambda _:'yes'
    proxy = Store()
    add_args = SimpleNamespace(months=12, user='John', passphrase='Setec Astronomy', machine='localhost', action='add', source=['libernet'], yes=False, no=False)
    libernet.backup.main(add_args, proxy)

    libernet.backup.USER_INPUT = lambda _:'no'
    proxy = Store()
    add_args = SimpleNamespace(months=12, user='John', passphrase='Setec Astronomy', machine='localhost', action='add', source=['libernet'], yes=False, no=False)
    try:
        libernet.backup.main(add_args, proxy)
        assert False, "We were supposed to fail"

    except AssertionError:
        pass

    libernet.backup.USER_INPUT = user_input


def test_max_like():
    max_like = libernet.disk.MAX_LIKE
    libernet.disk.MAX_LIKE = 5

    proxy = Store()
    add_args = SimpleNamespace(months=12, user='John', passphrase='Setec Astronomy', machine='localhost', action='add', source=['libernet'], yes=True)
    libernet.backup.main(add_args, proxy)
    backup_args = SimpleNamespace(months=12, user='John', passphrase='Setec Astronomy', machine='localhost', action='backup', source=[], yes=True)
    match_identifier = libernet.backup.get_similar_identifier(add_args)
    start_match_score = libernet.backup.target_match_score(match_identifier, proxy)

    for _ in range(0, libernet.disk.MAX_LIKE * 6):
        libernet.backup.main(backup_args, proxy)

    end_match_score = libernet.backup.target_match_score(match_identifier, proxy)
    assert start_match_score < end_match_score, f"{start_match_score} vs {end_match_score}"

    libernet.disk.MAX_LIKE = max_like


if __name__ == "__main__":
    test_max_like()
    """
    test_main()
    test_arg_processor()
    test_random_data()
    test_add_remove()
    test_list()
    test_input()
    test_no()
    test_arg_parser()
    """
