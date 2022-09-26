#!/usr/bin/env python3

""" Libernet bundle creator
"""

import argparse
import multiprocessing
import sys

import libernet.tools.bundle


def get_arg_parser():
    """Describe the command line arguments"""
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
        "-i",
        "--index",
        default=None,
        help="Name of the file in the root to use as the index",
    )
    parser.add_argument(
        "-d", "--dir", required=True, type=str, help="Directory to create a bundle for"
    )
    parser.add_argument("-p", "--previous", type=str, help="URL for previous version")
    parser.add_argument("-u", "--url", type=str, help="URL to restore")
    return parser


def handle_args(args):
    """respond to the arguments passed in"""
    if args.action == "store":
        results = libernet.tools.bundle.create(
            args.dir,
            args.storage,
            previous=args.previous,
            index=args.index,
            max_threads=multiprocessing.cpu_count(),
            verbose=args.verbose,
        )
        print(f"url: {results[0]}")
        return results[0]

    if args.action == "restore":
        if not args.url:  # not tested
            print("For restore you must specify a --url/-u")
            sys.exit(1)

        missing_blocks = libernet.tools.bundle.missing_blocks(args.url, args.storage)

        if missing_blocks:  # not tested
            print("The following blocks are missing and need to be loaded first")
            print("\t" + "\n\t".join(missing_blocks))
            sys.exit(1)

        bundle = libernet.tools.bundle.Path(args.url, args.storage)
        missing = bundle.missing_blocks()

        if missing:  # not tested
            print("Missing blocks:")
            print("\t" + "\n\t".join(missing))
            sys.exit(1)

        bundle.restore_file(args.dir)
        return True

    # not tested
    print("Unknown action: " + args.action)
    sys.exit(1)


def main():  # not tested
    """Entry point. Loop forever unless we are told not to."""
    handle_args(get_arg_parser().parse_args())


if __name__ == "__main__":  # not tested
    main()
