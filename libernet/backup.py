#!/usr/bin/env python3

"""
    Libernet Backup tool
"""


import os
import json
import argparse
import time
import zlib
import threading
import sys
import queue

from getpass import getpass

import keyring

import libernet.proxy
import libernet.bundle
import libernet.block
import libernet.message
import libernet.disk

from libernet.server import DEFAULT_PORT
from libernet.hash import sha256_data_identifier, identifier_match_score
from libernet.bundle import create_timestamp
from libernet.block import MATCH, COMPRESS_LEVEL
from libernet.url import for_data_block


DEFAULT_SERVER = "localhost"
DEFAULT_DAYS = 1.0
DEFAULT_MONTHS = 5
ENV_USER = "LIBERNETUSERNAME"
ENV_PASS = "LIBERNETPASSWORD"
KEY_SERVICE = "libernet"
KEY_USER = "username"
# TODO: use datetime delta  # pylint: disable=fixme
ONE_MONTH_IN_SECONDS = 365 * 24 * 60 * 60 / 12
USER_INPUT = input
PASSWORD_INPUT = getpass

# keys
TYPE = "type"
BACKUP = "backup"
TIMESTAMP = "timestamp"
USER = "user"
PASSWORD = "passphrase"
PREVIOUS = "previous"


def __load_settings_data(url: str, proxy, args) -> dict:
    try:
        data = libernet.block.fetch(
            url, proxy, was_similar=True, password=args.passphrase
        )
        uncompressed = zlib.decompress(data) if data else None
        results = json.loads(uncompressed) if uncompressed else {}
        assert results[TYPE] == BACKUP, results[TYPE]
        assert TIMESTAMP in results, results
        assert isinstance(results[TIMESTAMP], (float, int)), [results[TIMESTAMP]]
        assert results[USER] == args.user, f"{results[USER]} vs {args.user}"
        assert (
            results[PASSWORD] == args.passphrase
        ), f"{results[PASSWORD]} vs {args.passphrase}"

    except (
        AssertionError,
        KeyError,
        ValueError,
        zlib.error,
        json.decoder.JSONDecodeError,
        UnicodeDecodeError,
    ):
        return None

    return results


def get_similar_identifier(args, timestamp=time.time()) -> str:
    """Given the backup arguments, generate the ideal block identifier"""
    similar = f"USER:{args.user}@{time.strftime('%Y-%m', time.localtime(timestamp))}"
    return sha256_data_identifier(similar.encode("utf-8"))


def __merge_backups(identifiers: dict) -> dict:
    """take the latest backup for each path on each machine
    and take the latest of all keys (not including 'backup')
    """
    timeline = sorted(identifiers, key=lambda i: identifiers[i][TIMESTAMP])
    merged = {BACKUP: {}, PREVIOUS: list(identifiers)}

    for identifier in timeline:  # oldest to most recent
        fields = [k for k in identifiers[identifier] if k not in (BACKUP, PREVIOUS)]

        for field in fields:  # get latest version of all fields
            merged[field] = identifiers[identifier][field]

        machine_paths = [
            (m, p)
            for m in identifiers[identifier].get(BACKUP, {})
            for p in identifiers[identifier][BACKUP][m]
        ]

        for machine, path in machine_paths:
            merged[BACKUP][machine] = merged[BACKUP].get(machine, {})
            previous_info = merged[BACKUP][machine].get(path, {})
            previous_time = previous_info.get(TIMESTAMP, 0) if previous_info else 0
            this_info = identifiers[identifier][BACKUP][machine].get(path, {})
            this_time = this_info.get(TIMESTAMP, 0) if this_info else 0

            # if this is the latest version of this path, use it
            if this_time >= previous_time:
                merged[BACKUP][machine][path] = identifiers[identifier][BACKUP][
                    machine
                ][path]

    return merged


def __load_settings(args, proxy) -> dict:
    check = time.time()
    prompt = f"Unable to find backups in the last {args.months} months, create new? "
    possibilities = {}

    for months_ago in range(0, args.months):
        check = time.time() - months_ago * ONE_MONTH_IN_SECONDS
        similar_identifier = get_similar_identifier(args, check)  # check given month
        candidate_identifiers = proxy.like(
            for_data_block(similar_identifier, like=True)
        )
        possibilities.update(
            {
                i: __load_settings_data(i, proxy, args)
                for i in candidate_identifiers
                if i
            }
        )

    bye = [c for c, p in possibilities.items() if not p]
    bye.extend(i for c, p in possibilities.items() if p for i in p.get(PREVIOUS, []))

    for identifier in bye:
        del possibilities[identifier]

    found = __merge_backups(possibilities)

    if found.get(BACKUP, {}):
        return found

    if args.yes:
        print(prompt)
        print("Yes")

    elif args.no:
        print(prompt)
        print("No")
        assert found is not None, "Previous backups not found"

    else:
        answer = USER_INPUT(prompt)
        assert answer and answer.lower()[0] == "y"

    return {
        TYPE: BACKUP,
        TIMESTAMP: create_timestamp(),
        USER: args.user,
        PASSWORD: args.passphrase,
    }


def __add(args, settings: dict) -> bool:
    settings[BACKUP] = settings.get(BACKUP, {})
    settings[BACKUP][args.machine] = settings[BACKUP].get(args.machine, {})
    machine = settings[BACKUP][args.machine]
    changed = False

    for source in args.source:
        real_source = os.path.realpath(source)

        if real_source in machine:
            print(f"WARNING: we already have {real_source}")
        else:
            machine[real_source] = None
            changed = True

    return changed


def __remove(settings: dict, args) -> bool:
    settings[BACKUP] = settings.get(BACKUP, {})
    settings[BACKUP][args.machine] = settings[BACKUP].get(args.machine, {})
    machine = settings[BACKUP][args.machine]
    changed = False

    for source in args.source:
        real_source = os.path.realpath(source)

        if real_source in machine:
            print(f"No longer backup up: {real_source}")
            del machine[real_source]
            changed = True
        else:
            print(f"We are already not backing up: {real_source}")

    return changed


def target_match_score(similar_identifier: str, proxy) -> int:
    """Get the match score we need to beat to stay relevant"""
    existing = proxy.like(for_data_block(similar_identifier))

    if len(existing) < libernet.disk.MAX_LIKE:
        return MATCH

    return (
        min(
            identifier_match_score(libernet.url.parse(i)[0], similar_identifier)
            for i in existing
        )
        + 1
    )


def __save_backup(args, settings: dict, proxy):
    settings[TIMESTAMP] = create_timestamp()
    raw = json.dumps(settings, sort_keys=True, separators=(",", ":")).encode("utf-8")
    compressed = zlib.compress(
        raw, COMPRESS_LEVEL
    )  # password, so no natural compression
    similar = get_similar_identifier(args)
    score = target_match_score(similar, proxy)
    libernet.block.store(
        compressed, proxy, encrypt=args.passphrase, similar=similar, score=score
    )


def __list(settings: dict, args):
    sources = settings.get(BACKUP, {}).get(args.machine, {})

    if not sources:  # NOT TESTED
        print("No sources set to backup")
        print("Try `add --source ...`")
        return

    for path in sources:
        if sources[path] is None:  # NOT TESTED
            print(f"NOT BACKED UP: {path}")
        else:
            print(f"{path}")  # TODO: print backup timestamp  # pylint: disable=fixme


def __backup(settings: dict, proxy, args, message_center) -> bool:
    sources = settings.get(BACKUP, {}).get(args.machine, {})
    changed = False

    for source in sources:
        message_center.send(("source", source))

        if not os.path.isdir(source):  # NOT TESTED
            print(f"Directory not found: {source}")
            continue

        previous_url = sources[source]["url"] if sources[source] else None
        previous = (
            libernet.bundle.inflate(previous_url, proxy) if previous_url else None
        )
        start = time.perf_counter()
        url = libernet.bundle.create(
            source, proxy, previous, prior=previous_url, messages=message_center
        )
        print(f"duration: {time.perf_counter() - start:0.3f} seconds for {source}")
        sources[source] = {"url": url, TIMESTAMP: create_timestamp()}
        changed = True

    return changed


def __progress(message_center):
    channel = message_center.new_channel()
    need_newline = False
    last_file = None
    last_printed_file = None
    file_count = 0
    total_bytes = 0
    start_time = last_update = time.time()

    while message_center.active():
        try:
            message = channel.get(timeout=0.500)

        except queue.Empty:  # NOT TESTED
            continue

        if message is None:
            continue

        if message[0] == "source":
            if need_newline:  # NOT TESTED
                sys.stderr.write("\n")

            sys.stderr.write(message[1] + "\n")
            need_newline = False
            last_file = None
            continue

        if message[0] == "data":
            total_bytes += message[1]

        elif message[0] == "file":
            if need_newline:  # NOT TESTED
                sys.stderr.write("\n")

            file_count += 1
            last_file = message[1]
            need_newline = False

        if time.time() - last_update > 0.500:  # NOT TESTED
            if last_file != last_printed_file:
                if need_newline:
                    sys.stderr.write("\n")
                    need_newline = False

                sys.stderr.write(f"\t {last_file}\n")
                last_printed_file = last_file

            last_update = time.time()
            file_rate = file_count / (last_update - start_time)
            data_rate = total_bytes / (last_update - start_time) / 1024 / 1024
            file_info = f"{file_count:9,} files {file_rate:4.1f}/sec"
            data_info = (
                f"{total_bytes/1024/1024/1024:7.3f} GiB {data_rate:5.1f} MiB/sec"
            )
            sys.stderr.write("\r")
            sys.stderr.write(f"\t {file_info} {data_info}")
            need_newline = True

    if need_newline:  # NOT TESTED
        sys.stderr.write("\n")


def main(args, proxy=None):
    """main backup entry point"""
    proxy = libernet.proxy.Storage(args.server, args.port) if proxy is None else proxy
    message_center = libernet.message.Center()
    settings = __load_settings(args, proxy)
    changed = False
    rpt = threading.Thread(target=__progress, args=[message_center], daemon=True)
    rpt.start()

    if args.action == "add":
        changed = __add(args, settings)

    elif args.action == "list":
        __list(settings, args)

    elif args.action == "remove":
        changed = __remove(settings, args)

    elif args.action == "backup":
        changed = __backup(settings, proxy, args, message_center)

    if changed:
        __save_backup(args, settings, proxy)

    proxy.shutdown()
    message_center.shutdown()
    rpt.join()


def process_args(args, environment=None, key=keyring):
    """
    priority for username/passphrase
    1. command line
    2. environment (if specified)
    3. keychain (if specified)
    4. prompt
    If keychain specified but not found in keychain, it is added
    """
    environment = os.environ if environment is None else environment
    assert args.action != "list" or not args.source, "No --source for list"
    assert args.action in [
        "add",
        "remove",
        "list",
        "backup",
        "restore",
    ], f"unknown action: {args.action}"
    assert (
        args.action not in ("add", "remove") or args.source
    ), "You must specify --source"
    assert args.action == "restore" or args.destination is None
    key_user = key.get_password(KEY_SERVICE, KEY_USER) if args.keychain else None

    if args.user is None and args.environment:
        args.user = environment.get(ENV_USER, None)

    if args.user is None and args.keychain:
        args.user = key.get_password("libernet", "username")

    if args.user is None:
        args.user = USER_INPUT("Libernet account username: ")
        assert args.user, "You must specify a username"

    if args.keychain and key_user is None:
        key.set_password(KEY_SERVICE, KEY_USER, args.user)

    key_user = KEY_USER + "_" + args.user
    key_pass = key.get_password(KEY_SERVICE, key_user) if args.keychain else None

    if args.passphrase is None and args.environment:
        args.passphrase = environment.get(ENV_PASS, None)

    if args.passphrase is None and args.keychain:
        args.passphrase = key.get_password(KEY_SERVICE, key_user)

    if args.passphrase is None:
        args.passphrase = PASSWORD_INPUT("Libernet account pass phrase: ")
        assert args.passphrase, "You must specify a pass phrase"

    if args.keychain and key_pass is None:
        key.set_password(KEY_SERVICE, key_user, args.passphrase)

    return args


def get_arg_parser():
    """Describe the command line arguments"""
    parser = argparse.ArgumentParser(description="Libernet backup tool")
    parser.add_argument(
        "--server",
        default=DEFAULT_SERVER,
        help=f"Libernet server to connect to (default {DEFAULT_SERVER})",
    )
    parser.add_argument(
        "--port",
        help=f"The port to listen on (default {DEFAULT_PORT})",
        default=DEFAULT_PORT,
        type=int,
    )
    parser.add_argument(
        "--days",
        type=float,
        default=DEFAULT_DAYS,
        help="Warn if we cannot find a backup within this day range",
    )
    parser.add_argument(
        "--months",
        type=int,
        default=DEFAULT_MONTHS,
        help="How many months back to look for backups",
    )
    parser.add_argument(
        "--machine",
        type=str,
        required=True,
        help="The name of this machine",
    )
    parser.add_argument(
        "-u",
        "--user",
        help="Account username",
    )
    parser.add_argument(
        "-p",
        "--passphrase",
        help="Account passphrase",
    )
    parser.add_argument(
        "-y",
        "--yes",
        action="store_true",
        help="Reply 'Yes' to every question",
    )
    parser.add_argument(
        "-n",
        "--no",
        action="store_true",
        help="Reply 'No' to every question",
    )
    parser.add_argument(
        "-s",
        "--source",
        action="append",
        help="Source path to add, remove, list, backup, or restore",
    )
    parser.add_argument(
        "-d",
        "--destination",
        help="Destination path to restore",
    )
    parser.add_argument(
        "--keychain",
        action="store_true",
        help="Get Account username/passphrase from keychain (or store if not there)",
    )
    parser.add_argument(
        "--environment",
        action="store_true",
        help=f"Get Account username/passphrase from {ENV_USER} and {ENV_PASS}",
    )
    parser.add_argument("action", help="add, remove, list, backup, restore")
    return parser


if __name__ == "__main__":
    main(process_args(get_arg_parser().parse_args()))  # NOT TESTED
