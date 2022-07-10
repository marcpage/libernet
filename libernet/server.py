#!/usr/bin/env python3

import argparse
import libernet.plat.dirs

from flask import Flask, render_template, request, redirect, make_response


def create_app(storage_path):
    """create the flask app"""
    app = Flask(
        __name__,
        static_url_path="",
        static_folder=storage_path,
    )

    # Mark: Root

    @app.route("/")
    def home():
        return "<html><body>Welcome</body></html>"

    return app


def parse_args():
    """Parses and returns command line arguments."""

    parser = argparse.ArgumentParser(description="Libernet server")
    parser.add_argument(
        "-p",
        "--port",
        type=int,
        default=8000,
        help="The port to listen on (default 8000)",
    )
    parser.add_argument(
        "-s", "--storage", default=libernet.plat.dirs.pref_dir('libernet'), help="Directory to store data"
    )
    parser.add_argument("-d", "--debug", default=False, help="Run debug server.")
    args = parser.parse_args()

    return args


def main():
    """Entry point. Loop forever unless we are told not to."""

    args = parse_args()
    app = create_app(args.storage)
    app.run(host="0.0.0.0", debug=args.debug, port=args.port)


if __name__ == "__main__":
    main()
