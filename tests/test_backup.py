#!/usr/bin/env python3


import os
import time
import pprint
import tempfile

from types import SimpleNamespace
from random import randbytes
from tempfile import TemporaryDirectory

import libernet.block
import libernet.backup
import libernet.server
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

    def __str__(self):
        return str(self.data)


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


def create_file(directory:str, name:str, contents:str):
    with open(os.path.join(directory, name), 'w') as file:
        file.write(contents)

def validate_file(directory:str, name:str, contents:str) -> bool:
    with open(os.path.join(directory, name), 'r') as file:
        return contents == file.read()

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

    args = libernet.backup.process_args(SimpleNamespace(action='add', months=12, user='John', passphrase='Setec Astronomy', machine='localhost', source=['libernet', 'tests'], yes=True, destination=None, no=False, keychain=False, environment=False), environment=environment, key=keychain)
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
    assert args.passphrase == 'Setec Astronomy', args.passphrase
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
    update_period = libernet.backup.PROGRESS_UPDATE_PERIOD_IN_SECONDS
    libernet.backup.PROGRESS_UPDATE_PERIOD_IN_SECONDS = 0.100

    proxy = Store()
    other = Store()

    with TemporaryDirectory() as working_dir:
        for file_index in range(1, 10):
            with open(os.path.join(working_dir, f'big{file_index}.bin'), 'wb') as big_file:
                big_file.write(randbytes(2 * 1024 * 1024))  # 2 MiB file of random data

        add_args = SimpleNamespace(months=12, user='John', passphrase='Setec Astronomy', machine='localhost', action='add', source=['libernet', 'tests', working_dir], yes=True)
        libernet.backup.main(add_args, proxy)
        libernet.backup.main(add_args, proxy)
        libernet.backup.main(add_args, other)
        backup_args = SimpleNamespace(months=12, user='John', passphrase='Setec Astronomy', machine='localhost', action='backup', source=[], yes=True)
        libernet.backup.main(backup_args, other)
        libernet.backup.main(backup_args, proxy)
        libernet.backup.main(backup_args, other)
        libernet.backup.main(backup_args, proxy)
        proxy.data.update(other.data)  # simulate backing up multiple machines and merging them
        libernet.backup.main(add_args, proxy)

    libernet.backup.PROGRESS_UPDATE_PERIOD_IN_SECONDS = update_period


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
    list_args = SimpleNamespace(months=12, user='John', passphrase='Setec Astronomy', machine='localhost', action='list', source=[], yes=True)
    add_args = SimpleNamespace(months=12, user='John', passphrase='Setec Astronomy', machine='localhost', action='add', source=['libernet'], yes=True)
    backup_args = SimpleNamespace(months=12, user='John', passphrase='Setec Astronomy', machine='localhost', action='backup', source=[], yes=True)
    libernet.backup.main(list_args, proxy)
    libernet.backup.main(add_args, proxy)
    libernet.backup.main(list_args, proxy)
    libernet.backup.main(backup_args, proxy)
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


def test_no_dir():
    proxy = Store()
    list_args = SimpleNamespace(months=12, user='John', passphrase='Setec Astronomy', machine='localhost', action='list', source=[], yes=True)
    add_args = SimpleNamespace(months=12, user='John', passphrase='Setec Astronomy', machine='localhost', action='add', source=['xyz_does-not-exist-xyz'], yes=True)
    backup_args = SimpleNamespace(months=12, user='John', passphrase='Setec Astronomy', machine='localhost', action='backup', source=[], yes=True)
    libernet.backup.main(list_args, proxy)
    libernet.backup.main(add_args, proxy)
    libernet.backup.main(list_args, proxy)
    libernet.backup.main(backup_args, proxy)
    libernet.backup.main(list_args, proxy)


def test_restore_simple():
    proxy = Store()

    with TemporaryDirectory() as working_dir:
        source_dir_1 = os.path.join(working_dir, 'dir1')
        os.makedirs(source_dir_1)
        create_file(source_dir_1, 'file1.txt', 'file1 contents')
        create_file(source_dir_1, 'file2.txt', 'file2 contents')

        dest_dir_1 = os.path.join(working_dir, 'restored1')

        add_args = SimpleNamespace(months=12, user='John', passphrase='Setec Astronomy', machine='localhost', action='add', source=[source_dir_1], yes=True)
        backup_args = SimpleNamespace(months=12, user='John', passphrase='Setec Astronomy', machine='localhost', action='backup', source=[])
        restore_args = SimpleNamespace(months=12, user='John', passphrase='Setec Astronomy', machine='localhost', action='restore', source=[], destination=dest_dir_1)

        libernet.backup.main(add_args, proxy)
        libernet.backup.main(restore_args, proxy)
        assert not os.path.isdir(dest_dir_1)
        libernet.backup.main(backup_args, proxy)
        libernet.backup.main(restore_args, proxy)

        assert validate_file(dest_dir_1, 'file1.txt', 'file1 contents')
        assert validate_file(dest_dir_1, 'file2.txt', 'file2 contents')


def test_restore_to_source():
    proxy = Store()

    with TemporaryDirectory() as working_dir:
        source_dir_1 = os.path.join(working_dir, 'dir1')
        os.makedirs(source_dir_1)
        create_file(source_dir_1, 'file1.txt', 'file1 contents')
        create_file(source_dir_1, 'file2.txt', 'file2 contents')
        add_args = SimpleNamespace(months=12, user='John', passphrase='Setec Astronomy', machine='localhost', action='add', source=[source_dir_1], yes=True)
        libernet.backup.main(add_args, proxy)
        backup_args = SimpleNamespace(months=12, user='John', passphrase='Setec Astronomy', machine='localhost', action='backup', source=[])
        libernet.backup.main(backup_args, proxy)
        create_file(source_dir_1, 'file1.txt', 'file1 bad contents')
        create_file(source_dir_1, 'file2.txt', 'file2 bad contents')
        restore_args = SimpleNamespace(months=12, user='John', passphrase='Setec Astronomy', machine='localhost', action='restore', source=[], destination=None)
        libernet.backup.main(restore_args, proxy)
        assert validate_file(source_dir_1, 'file1.txt', 'file1 contents')
        assert validate_file(source_dir_1, 'file2.txt', 'file2 contents')


def test_restore_2_dirs():
    proxy = Store()

    with TemporaryDirectory() as working_dir:
        source_dir_1 = os.path.join(working_dir, 'test', 'dir1')
        source_dir_2 = os.path.join(working_dir, 'john', 'dir2')
        source_dir_3 = os.path.join(working_dir, 'what', 'dir3')
        os.makedirs(source_dir_1)
        os.makedirs(source_dir_2)
        dest_dir_1 = os.path.join(working_dir, 'restored1')
        create_file(source_dir_1, 'file1.txt', 'file1 contents')
        create_file(source_dir_2, 'file2.txt', 'file2 contents')
        add_args = SimpleNamespace(months=12, user='John', passphrase='Setec Astronomy', machine='localhost', action='add', source=[source_dir_1, source_dir_2], yes=True)
        libernet.backup.main(add_args, proxy)
        backup_args = SimpleNamespace(months=12, user='John', passphrase='Setec Astronomy', machine='localhost', action='backup', source=[])
        libernet.backup.main(backup_args, proxy)
        restore_args = SimpleNamespace(months=12, user='John', passphrase='Setec Astronomy', machine='localhost', action='restore', source=[], destination=dest_dir_1)
        libernet.backup.main(restore_args, proxy)
        assert validate_file(os.path.join(dest_dir_1, 'dir1'), 'file1.txt', 'file1 contents')
        assert validate_file(os.path.join(dest_dir_1, 'dir2'), 'file2.txt', 'file2 contents')
        restore_args = SimpleNamespace(months=12, user='John', passphrase='Setec Astronomy', machine='localhost', action='restore', source=[source_dir_3], destination=dest_dir_1)
        libernet.backup.main(restore_args, proxy)


def test_restore_2_dirs_same_name():
    proxy = Store()

    with TemporaryDirectory() as working_dir:
        source_dir_1 = os.path.join(working_dir, 'test', 'dir')
        source_dir_2 = os.path.join(working_dir, 'john', 'dir')
        os.makedirs(source_dir_1)
        os.makedirs(source_dir_2)
        dest_dir_1 = os.path.join(working_dir, 'restored1')
        create_file(source_dir_1, 'file1.txt', 'file1 contents')
        create_file(source_dir_2, 'file2.txt', 'file2 contents')
        add_args = SimpleNamespace(months=12, user='John', passphrase='Setec Astronomy', machine='localhost', action='add', source=[source_dir_1, source_dir_2], yes=True)
        libernet.backup.main(add_args, proxy)
        backup_args = SimpleNamespace(months=12, user='John', passphrase='Setec Astronomy', machine='localhost', action='backup', source=[])
        libernet.backup.main(backup_args, proxy)
        restore_args = SimpleNamespace(months=12, user='John', passphrase='Setec Astronomy', machine='localhost', action='restore', source=[], destination=dest_dir_1)
        libernet.backup.main(restore_args, proxy)
        file1_dir = None
        file2_dir = None

        for root, dirs, files in os.walk(dest_dir_1):
            if 'file1.txt' in files:
                validate_file(root, 'file1.txt', 'file1 contents')
                file1_dir = root

            if 'file2.txt' in files:
                validate_file(root, 'file2.txt', 'file2 contents')
                file2_dir = root

        assert os.path.split(file1_dir)[1] == os.path.split(file2_dir)[1]
        file1_dir_dir_dir = os.path.split(os.path.split(file1_dir)[0])[0]
        file2_dir_dir_dir = os.path.split(os.path.split(file2_dir)[0])[0]
        assert file1_dir_dir_dir == file2_dir_dir_dir, f"{file1_dir_dir_dir}\nvs\n{file2_dir_dir_dir}"


def test_restore_missing_blocks():
    proxy = Store()

    with TemporaryDirectory() as working_dir:
        source_dir_1 = os.path.join(working_dir, 'dir1')
        os.makedirs(source_dir_1)
        create_file(source_dir_1, 'file1.txt', 'file1 contents')
        create_file(source_dir_1, 'file2.txt', 'file2 contents')

        dest_dir_1 = os.path.join(working_dir, 'restored1')

        add_args = SimpleNamespace(months=12, user='John', passphrase='Setec Astronomy', machine='localhost', action='add', source=[source_dir_1], yes=True)
        backup_args = SimpleNamespace(months=12, user='John', passphrase='Setec Astronomy', machine='localhost', action='backup', source=[])
        restore_args = SimpleNamespace(months=12, user='John', passphrase='Setec Astronomy', machine='localhost', action='restore', source=[], destination=dest_dir_1)

        libernet.backup.main(add_args, proxy)
        keepers = {k[:11] for k in proxy.data}  # keep the settings files
        libernet.backup.main(backup_args, proxy)

        for block in list(proxy.data.keys()):
            if block[:11] not in keepers:
                del proxy.data[block]

        libernet.backup.main(restore_args, proxy)
        assert not os.path.isdir(dest_dir_1)


def test_load_settings_port():
    with tempfile.TemporaryDirectory() as storage:
        args = SimpleNamespace(storage=storage, port=None, server=None, days=None, months=None, machine=None)
        output = libernet.backup.load_settings(args, input_func=lambda _:'1234')
        assert output.port == libernet.server.DEFAULT_PORT, f"{output.port} vs {libernet.server.DEFAULT_PORT}"
        output = libernet.backup.load_settings(args, input_func=lambda _:'5678')
        assert output.port == libernet.server.DEFAULT_PORT, f"{output.port} vs {libernet.server.DEFAULT_PORT}"
        args.port = 8087
        output = libernet.backup.load_settings(args, input_func=lambda _:'5678')
        assert output.port == 8087, output.port
        args.port = None
        output = libernet.backup.load_settings(args, input_func=lambda _:'5678')
        assert output.port == 8087, output.port

    with tempfile.TemporaryDirectory() as storage:
        args = SimpleNamespace(storage=storage, port=None, server=None, days=None, months=None, machine=None)
        output = libernet.server.load_settings(args, input_func=lambda _:'1234')
        assert output.port == libernet.server.DEFAULT_PORT, f"{output.port} vs {libernet.server.DEFAULT_PORT}"
        output = libernet.backup.load_settings(args, input_func=lambda _:'4321')
        assert output.port == libernet.server.DEFAULT_PORT, f"{output.port} vs {libernet.server.DEFAULT_PORT}"
        output = libernet.backup.load_settings(args, input_func=lambda _:'5678')
        assert output.port == libernet.server.DEFAULT_PORT, f"{output.port} vs {libernet.server.DEFAULT_PORT}"
        args.port = 8087
        output = libernet.backup.load_settings(args, input_func=lambda _:'5678')
        assert output.port == 8087, output.port
        args.port = None
        output = libernet.backup.load_settings(args, input_func=lambda _:'5678')
        assert output.port == 8087, output.port


def test_load_settings_server():
    with tempfile.TemporaryDirectory() as storage:
        args = SimpleNamespace(storage=storage, port=None, server=None, days=None, months=None, machine=None)
        output = libernet.backup.load_settings(args, input_func=lambda _:'88')
        assert output.port == libernet.server.DEFAULT_PORT, f"{output.port} vs {libernet.server.DEFAULT_PORT}"
        assert output.server == libernet.backup.DEFAULT_SERVER, f"{output.server} vs {libernet.backup.DEFAULT_SERVER}"
        assert output.machine == '88', output.machine
        assert output.days == libernet.backup.DEFAULT_DAYS, f"{output.days} vs {libernet.backup.DEFAULT_DAYS}"
        assert output.months == libernet.backup.DEFAULT_MONTHS, f"{output.months} vs {libernet.backup.DEFAULT_MONTHS}"

        args = SimpleNamespace(storage=storage, port=92, server="testing", days=None, months=None, machine=None)
        output = libernet.backup.load_settings(args, input_func=lambda _:'88')
        assert output.port == 92, f"{output.port} vs {92}"
        assert output.server == 'testing', output.server
        assert output.machine == '88', output.machine
        assert output.days == libernet.backup.DEFAULT_DAYS, f"{output.days} vs {libernet.backup.DEFAULT_DAYS}"
        assert output.months == libernet.backup.DEFAULT_MONTHS, f"{output.months} vs {libernet.backup.DEFAULT_MONTHS}"

        args = SimpleNamespace(storage=storage, port=None, server=None, days=None, months=None, machine=None)
        output = libernet.backup.load_settings(args, input_func=lambda _:'88')
        assert output.port == 92, output.port
        assert output.server == "testing", output.server
        assert output.machine == '88', output.machine
        assert output.days == libernet.backup.DEFAULT_DAYS, f"{output.days} vs {libernet.backup.DEFAULT_DAYS}"
        assert output.months == libernet.backup.DEFAULT_MONTHS, f"{output.months} vs {libernet.backup.DEFAULT_MONTHS}"

        args = SimpleNamespace(storage=storage, port=None, server="testing", days=None, months=None, machine=None)
        output = libernet.backup.load_settings(args, input_func=lambda _:'88')
        assert output.port == 92, f"{output.port} vs {92}"
        assert output.server == 'testing', output.server
        assert output.machine == '88', output.machine
        assert output.days == libernet.backup.DEFAULT_DAYS, f"{output.days} vs {libernet.backup.DEFAULT_DAYS}"


def test_load_settings():
    with tempfile.TemporaryDirectory() as storage:
        args = SimpleNamespace(storage=storage, port=None, server=None, days=None, months=None, machine=None)
        output = libernet.backup.load_settings(args, input_func=lambda _:'88')
        assert output.port == libernet.server.DEFAULT_PORT, f"{output.port} vs {libernet.server.DEFAULT_PORT}"
        assert output.server == libernet.backup.DEFAULT_SERVER, f"{output.server} vs {libernet.backup.DEFAULT_SERVER}"
        assert output.machine == '88', output.machine
        assert output.days == libernet.backup.DEFAULT_DAYS, f"{output.days} vs {libernet.backup.DEFAULT_DAYS}"

        args = SimpleNamespace(storage=storage, port=55, server="testing", days=3, months=12, machine="lolipop")
        output = libernet.backup.load_settings(args, input_func=lambda _:'88')
        assert output.port == 55, output.port
        assert output.server == "testing", output.server
        assert output.machine == 'lolipop', output.machine
        assert output.days == 3, output.days

        args = SimpleNamespace(storage=storage, port=None, server=None, days=None, months=None, machine=None)
        output = libernet.backup.load_settings(args, input_func=lambda _:'88')
        assert output.port == 55, output.port
        assert output.server == "testing", output.server
        assert output.machine == 'lolipop', output.machine
        assert output.days == 3, output.days


if __name__ == "__main__":
    test_arg_processor()
    test_load_settings_server()
    test_load_settings()
    test_load_settings_port()
    test_restore_missing_blocks()
    test_restore_simple()
    test_restore_2_dirs()
    test_restore_2_dirs_same_name()
    test_restore_to_source()
    test_main()
    test_arg_parser()
    test_random_data()
    test_add_remove()
    test_list()
    test_no()
    test_input()
    test_max_like()
    test_no_dir()
