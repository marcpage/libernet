#!/usr/bin/env python3

import argparse
import libernet.plat.dirs
import os

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

    @app.route("/sha256/<identifier>")
    def sha256(identifier):
        return f"<html><body>Identifier {identifier}</body></html>"

    # Mark: v1 API

    @app.route("/api/v1/backup/add")
    def add_backup():
        return "{}"

    @app.route("/api/v1/backup/remove")
    def remove_backup():
        return "{}"

    @app.route("/api/v1/backup/list")
    def list_backups():
        return "[]"

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
    parser.add_argument("-d", "--debug", default=False, action="store_true", help="Run debug server.")
    args = parser.parse_args()

    libernet.plat.dirs.make_dirs(os.path.join(args.storage, 'web'))
    libernet.plat.dirs.make_dirs(os.path.join(args.storage, 'upload'))
    return args


def main():
    """Entry point. Loop forever unless we are told not to."""

    args = parse_args()
    app = create_app(args.storage)
    app.run(host="0.0.0.0", debug=args.debug, port=args.port)


if __name__ == "__main__":
    main()
