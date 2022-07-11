#!/usr/bin/env python3

""" Libernet bundle creator
"""

import argparse

import libernet.tools.bundle


def parse_args():
    """Parses and returns command line arguments."""
    parser = argparse.ArgumentParser(description="Libernet bundle creator")
    parser.add_argument(
        "action",
        choices=["store", "restore"],
        help="Action to perform",
    )
    parser.add_argument(
        "-s",
        "--storage",
        default=libernet.plat.dirs.pref_dir("libernet"),
        help="Directory to store/retrieve data",
    )
    parser.add_argument(
        "-d", "--dir", required=True, type=str, help="Directory to create a bundle for"
    )
    parser.add_argument("-p", "--previous", type=str, help="URL for previous version")
    args = parser.parse_args()
    return args


def main():
    """Entry point. Loop forever unless we are told not to."""
    args = parse_args()
    results = libernet.tools.bundle.create(args.dir, args.storage, args.previous)
    print(f"url: {results[0]}")


if __name__ == "__main__":
    main()
