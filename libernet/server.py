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
import libernet.tools.bundle
import libernet.tools.settings


def create_app(storage_path):
    """create the flask app"""
    app = flask.Flask(__name__)
    settings = libernet.tools.settings.App(storage_path)

    def forbidden():
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
        return forbidden()

    @app.route("/sha256/<path:path>")
    def sha256(path):
        full_url = f"/sha256/{path}"
        block_identifier, block_key, path_in_bundle = libernet.tools.block.validate_url(
            full_url
        )
        logging.debug(
            "full_url='%s' block_identifier='%s' block_key='%s' path_in_bundle='%s'",
            full_url,
            block_identifier,
            block_key,
            path_in_bundle,
        )
        local_request = libernet.plat.network.is_on_machine(flask.request.remote_addr)

        if block_key is not None and not local_request:
            logging.debug("block_key is not None and not local_request")
            return forbidden()

        if path_in_bundle is not None:
            bundle = libernet.tools.bundle.Path(full_url, settings.storage())
            bundle_path = os.path.join(
                settings.storage(), "web", bundle.relative_path(just_bundle=True)
            )
            item_path = os.path.join(bundle_path, path_in_bundle)
            already_exists = os.path.isfile(item_path)
            missing = (
                bundle.missing_blocks(path_in_bundle) if not already_exists else []
            )
            logging.debug(
                "path_in_bundle='%s' item_path='%s' already_exists=%s missing=%s",
                path_in_bundle,
                item_path,
                already_exists,
                missing,
            )

            if not missing and not already_exists:
                already_exists = bundle.restore_file(bundle_path)
                logging.debug(
                    "not missing and not already_exists: already_exists=%s",
                    already_exists,
                )

                if not already_exists:
                    # the path was not found
                    return (
                        f"<html><body>{full_url} not found in bundle</body></html>",
                        404,
                    )  # File not found

            if already_exists:
                return flask.send_file(item_path)

            contents = None  # get full contents of file then send file

        else:
            try:
                contents = libernet.tools.block.get_contents(
                    settings.storage(), block_identifier, block_key
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
        if not libernet.plat.network.is_on_machine(flask.request.remote_addr):
            return forbidden()
        return "{}"

    @app.route("/api/v1/backup/remove")
    def remove_backup():
        if not libernet.plat.network.is_on_machine(flask.request.remote_addr):
            return forbidden()
        return "{}"

    @app.route("/api/v1/backup/list")
    def list_backups():
        if not libernet.plat.network.is_on_machine(flask.request.remote_addr):
            return forbidden()
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
    log_level = logging.DEBUG if args.debug else logging.WARNING
    logging.basicConfig(filename=os.path.join(args.storage, "log.txt"), level=log_level)
    app = create_app(args.storage)
    app.run(host="0.0.0.0", debug=args.debug, port=args.port)


if __name__ == "__main__":
    main()
