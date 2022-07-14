#!/usr/bin/env python3

""" Libernet server
"""

import argparse
import os
import logging
import flask
import io

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

    @app.route("/sha256/<identifier>")
    def sha256(identifier):
        print(f"identifier = {identifier}")
        contents = libernet.tools.block.get_contents(app.static_folder, identifier)

        if contents is None:
            # temporarily not available
            # start requesting this block
            return "<html><body>not available yet</body></html>", 409  # Conflict

        content_file = io.BytesIO(contents)
        # last_modified = datetime.datetime, int, float
        return flask.send_file(
            content_file,
            mimetype="application/octet-stream",
            as_attachment=True,
            max_age=365 * 24 * 60 * 60,
            download_name=identifier,
            etag=identifier,
        )

    """
    @app.route("/sha256/<identifier>/aes256/<key>")
    def sha256(identifier, key):
        contents = libernet.tools.block.get_contents(static_folder, identifier, key)

        if contents is None:
            # temporarily not available
            # start requesting this block
            return "<html><body>not available yet</body></html>", 409  # Conflict

        content_file = io.BytesIO(contents)
        # last_modified = datetime.datetime, int, float
        return flask.send_file(content_file, mimetype='application/octet-stream', as_attachment=True, max_age=365*24*60*60, download_name=identifier, etag=identifier)
    """

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
