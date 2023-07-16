#!/usr/bin/env python3

"""
    Libernet Server
"""

import os
import logging
import argparse
import json

import flask

import libernet.url

from libernet.disk import Storage
from libernet.url import SHA256, LIKE

DATA_MIMETYPE = "application/octet-stream"
JSON_MIMETYPE = "application/json"
DEFAULT_PORT = 8042
DEFAULT_STORAGE = os.path.join(os.environ["HOME"], ".libernet")


def create_app(storage: Storage):
    """Creates the Flask app"""
    app = flask.Flask(__name__)

    @app.route(f"/{SHA256}/<path:path>", methods=["GET"])
    def get_sha256(path: str):
        """Return the requested data"""
        assert path in flask.request.path, f"{path} vs {flask.request.path}"
        identifier, key, _, kind = libernet.url.parse(flask.request.path)
        assert key is None, flask.request.path
        data_block_url = libernet.url.for_data_block(identifier, kind == LIKE)

        if kind == LIKE:
            found = storage.like(data_block_url)
            response = flask.Response(json.dumps(found), mimetype=JSON_MIMETYPE)
            response.status = 200 if len(found) > 0 else 404
            return response

        contents = storage.get(data_block_url)

        if contents is None:
            return "Data not currently on node", 504

        response = flask.Response(contents, mimetype=DATA_MIMETYPE)
        response.status = 200
        return response

    @app.route(f"/{SHA256}/<identifier>", methods=["PUT"])
    def put_sha256(identifier: str):
        """Return the requested data"""
        storage[libernet.url.for_data_block(identifier)] = flask.request.data
        return "data received", 200

    return app


def serve(args):  # NOT TESTED - no running server during tests
    """Start the libernet web server"""
    log_path = os.path.join(args.storage, "log.txt")
    os.makedirs(os.path.split(log_path)[0], exist_ok=True)
    log_level = logging.DEBUG if args.debug else logging.WARNING
    logging.basicConfig(filename=log_path, level=log_level)
    storage = Storage(args.storage)
    app = create_app(storage)
    app.run(host="0.0.0.0", debug=args.debug, port=args.port)


def get_arg_parser():
    """Describe the command line arguments"""
    parser = argparse.ArgumentParser(description="Libernet server")
    parser.add_argument(
        "-p",
        "--port",
        type=int,
        default=DEFAULT_PORT,
        help=f"The port to listen on (default {DEFAULT_PORT})",
    )
    parser.add_argument(
        "-s",
        "--storage",
        default=DEFAULT_STORAGE,
        help=f"Directory to store data (default {DEFAULT_STORAGE})",
    )
    parser.add_argument(
        "-d", "--debug", default=False, action="store_true", help="Run debug serve."
    )
    return parser


if __name__ == "__main__":
    serve(get_arg_parser().parse_args())  # NOT TESTED - no running server during tests
