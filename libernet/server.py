#!/usr/bin/env python3

""" Libernet server
"""

import argparse
import os
import logging
import zipfile
import time
import threading

import flask

import libernet.tools.block
import libernet.plat.dirs
import libernet.plat.network
import libernet.tools.bundle
import libernet.tools.settings
import libernet.plat.files
import libernet.tools.contents


BROWSER_OPEN_DELAY_IN_SECONDS = 21.000
LOG_FILE_MAX_SIZE = 1024 * 1024  # 1 MiB


def create_app(storage_path, key_size=4096):
    """create the flask app"""
    app = flask.Flask(__name__)
    settings = libernet.tools.settings.App(storage_path, key_size=key_size)

    # Mark: Root

    @app.route("/")
    def home():
        return libernet.tools.contents.home(
            flask.request.remote_addr, flask.request.environ
        )

    @app.route("/sha256/<identifier>", methods=["PUT"])
    def put_sha256(identifier):
        response = flask.Response(
            "<html><body>Thank you for sending the data</body></html>"
        )
        sender = flask.request.headers.get(libernet.tools.settings.HTTP_AUTHOR)
        search_dir = os.path.join(
            settings.storage(), libernet.tools.block.UPLOAD_SUBDIR, sender
        )
        path = (
            libernet.tools.block.block_dir(search_dir, identifier, full=True) + ".raw"
        )
        response.headers[
            libernet.tools.settings.HTTP_AUTHOR
        ] = settings.identity().identifier()
        response.status = 200
        return response

    @app.route("/sha256/<path:path>")
    def sha256(path):
        result = libernet.tools.contents.sha256(
            path, flask.request.remote_addr, settings.storage()
        )

        if result[0] == "file":
            return flask.send_file(result[1], **result[2])

        return result

    # Mark: v1 API

    @app.route("/api/v1/backup/add")
    def add_backup():
        if not libernet.plat.network.is_on_machine(flask.request.remote_addr):
            return libernet.tools.contents.forbidden()
        return "{}"

    @app.route("/api/v1/backup/remove")
    def remove_backup():
        if not libernet.plat.network.is_on_machine(flask.request.remote_addr):
            return libernet.tools.contents.forbidden()
        return "{}"

    @app.route("/api/v1/backup/list")
    def list_backups():
        if not libernet.plat.network.is_on_machine(flask.request.remote_addr):
            return libernet.tools.contents.forbidden()
        return "[]"

    return app


def get_arg_parser():
    """Describe the command line arguments"""
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
    parser.add_argument(
        "-b",
        "--background",
        default=False,
        action="store_true",
        help="Do not launch web browser.",
    )
    return parser


def delayed_open_browser(url, delay_in_seconds):
    """wait and then open the url in the user's browser"""
    time.sleep(delay_in_seconds)
    libernet.plat.files.open_url(url)


def rotate_log(log_path):
    """If the log file gets too big, archive the log file"""
    if os.path.isfile(log_path) and os.path.getsize(log_path) > LOG_FILE_MAX_SIZE:
        archive_path = f"{log_path}_{time.strftime('%Y%m%d%H%M%S')}.zip"

        with zipfile.ZipFile(
            archive_path, "w", zipfile.ZIP_DEFLATED, False, 9
        ) as archive:
            archive.write(log_path, "log.txt")

        os.unlink(log_path)


def serve(port, storage, debug, key_size=4096):
    """Start the libernet web server"""
    log_path = os.path.join(storage, "log.txt")
    rotate_log(log_path)
    log_level = logging.DEBUG if debug else logging.WARNING
    logging.basicConfig(filename=log_path, level=log_level)
    app = create_app(storage, key_size=key_size)

    if debug:  # NOT TESTED
        threading.Thread(
            target=delayed_open_browser,
            args=(f"http://localhost:{port}", BROWSER_OPEN_DELAY_IN_SECONDS),
            daemon=True,
        ).start()

    app.run(host="0.0.0.0", debug=debug, port=port)


def handle_args(args, key_size=4096):
    """respond to the arguments passed in"""
    libernet.plat.dirs.make_dirs(
        os.path.join(args.storage, libernet.tools.block.WEB_SUBDIR)
    )
    libernet.plat.dirs.make_dirs(
        os.path.join(args.storage, libernet.tools.block.UPLOAD_SUBDIR)
    )
    serve(args.port, args.storage, args.debug, key_size=key_size)


def main():  # NOT TESTED
    """Entry point. Loop forever unless we are told not to."""
    handle_args(get_arg_parser().parse_args())


if __name__ == "__main__":  # NOT TESTED
    main()
