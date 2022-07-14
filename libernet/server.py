#!/usr/bin/env python3

""" Libernet server
"""

import argparse
import os
import logging
import io
import zlib

import flask

import libernet.tools.block
import libernet.plat.dirs
import libernet.plat.network


def create_app(storage_path):
    """create the flask app"""
    app = flask.Flask(
        __name__,
        static_url_path="",
        static_folder=storage_path,
    )

    # Mark: Root

    @app.route("/")
    def home():
        if libernet.plat.network.is_on_machine(flask.request.remote_addr):
            return f"""
<html>
    <body>
        <h1>Welcome</h1>
        {flask.request.remote_addr}
    </body>
</html>"""
        return (
            """
<html>
    <body>
        <h1>Forbidden</h1>
        You do not have access to this page
    </body>
</html>""",
            403,
        )  # Forbidden

    @app.route("/sha256/<path:path>")
    def sha256(path):
        print(f"path = {path}")
        print(f"url = {flask.request.url}")
        print(f"path = {flask.request.path}")
        print(f"base_url = {flask.request.base_url}")
        print(f"form = {flask.request.form}")
        print(f"query_string = {flask.request.query_string}")
        print(f"full_path = {flask.request.full_path}")
        print(f"args = {flask.request.args}")
        full_url = f"/sha256/{path}"
        block_identifier, block_key, bundle_path = libernet.tools.block.validate_url(
            full_url
        )
        print(bundle_path)
        try:
            contents = libernet.tools.block.get_contents(
                app.static_folder, block_identifier, block_key
            )

        except zlib.error:
            # the key field is probably incorrect
            return (
                f"<html><body>{full_url} unable to decrypt</body></html>",
                400,
            )  # Bad request

        if contents is None:
            # temporarily not available
            # start requesting this block
            return (
                f"<html><body>{full_url} not available yet</body></html>",
                409,
            )  # Conflict

        content_file = io.BytesIO(contents)
        # last_modified = datetime.datetime, int, float
        return flask.send_file(
            content_file,
            mimetype="application/octet-stream",
            as_attachment=True,
            max_age=365 * 24 * 60 * 60,
            download_name=block_identifier if block_key is None else block_key,
            etag=block_identifier if block_key is None else block_key,
        )

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
        "-s",
        "--storage",
        default=libernet.plat.dirs.pref_dir("libernet"),
        help="Directory to store data",
    )
    parser.add_argument(
        "-d", "--debug", default=False, action="store_true", help="Run debug server."
    )
    args = parser.parse_args()

    libernet.plat.dirs.make_dirs(os.path.join(args.storage, "web"))
    libernet.plat.dirs.make_dirs(os.path.join(args.storage, "upload"))
    return args


def main():
    """Entry point. Loop forever unless we are told not to."""

    args = parse_args()
    logging.basicConfig(filename=os.path.join(args.storage, "log.txt"))
    app = create_app(args.storage)
    app.run(host="0.0.0.0", debug=args.debug, port=args.port)


if __name__ == "__main__":
    main()
