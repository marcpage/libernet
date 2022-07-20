#!/usr/bin/env python3

""" Libernet bundle creator
"""

import argparse
import multiprocessing
import sys

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
        "-v",
        "--verbose",
        action="store_true",
        default=False,
        help="Dump files while processing",
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
    parser.add_argument("-u", "--url", type=str, help="URL to restore")
    args = parser.parse_args()
    return args


def main():
    """Entry point. Loop forever unless we are told not to."""
    args = parse_args()

    if args.action == "store":
        results = libernet.tools.bundle.create(
            args.dir,
            args.storage,
            args.previous,
            max_threads=multiprocessing.cpu_count(),
            verbose=args.verbose,
        )
        print(f"url: {results[0]}")

    elif args.action == "restore":
        if not args.url:
            print("For restore you must specify a --url/-u")
            sys.exit(1)

        missing_blocks = libernet.tools.bundle.missing_blocks(args.url, args.storage)

        if missing_blocks:
            print("The following blocks are missing and need to be loaded first")
            print("\t" + "\n\t".join(missing_blocks))

        else:
            bundle = libernet.tools.bundle.Path(args.url, args.storage)
            missing = bundle.missing_blocks()

            if missing:
                print("Missing blocks:")
                print("\t" + "\n\t".join(missing))
            else:
                bundle.restore_file(args.dir)


if __name__ == "__main__":
    main()
