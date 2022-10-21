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
        return (0, results[0])

    if args.action == "restore":
        if not args.url:
            print("For restore you must specify a --url/-u")
            return (1, None)

        missing_blocks = libernet.tools.bundle.missing_blocks(args.url, args.storage)

        if missing_blocks:
            print("The following blocks are missing and need to be loaded first")
            print("\t" + "\n\t".join(missing_blocks))
            return (1, None)

        bundle = libernet.tools.bundle.Path(args.url, args.storage)
        missing = bundle.missing_blocks()

        if missing:  # NOT TESTED, should never happen
            print("Missing blocks:")
            print("\t" + "\n\t".join(missing))
            return (1, None)

        bundle.restore_file(args.dir)
        return (0, True)

    print("Unknown action: " + args.action)
    return (1, None)


def main():  # NOT TESTED
    """Entry point. Loop forever unless we are told not to."""
    sys.exit(handle_args(get_arg_parser().parse_args())[0])


if __name__ == "__main__":  # NOT TESTED
    main()
