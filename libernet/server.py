#!/usr/bin/env python3

"""
    Libernet Server
"""

import os
import logging
import argparse
import json

import flask

from libernet.hash import identifier_match_score, IDENTIFIER_SIZE


DATA_MIMETYPE = "application/octet-stream"
JSON_MIMETYPE = "application/json"
DEFAULT_PORT = 8042
DEFAULT_STORAGE = os.path.join(os.environ["HOME"], ".libernet")
MAX_LIKE = 100


def create_app(args):
    """Creates the Flask app"""
    app = flask.Flask(__name__)
    storage_path = os.path.join(args.storage, "sha256")

    @app.route("/sha256/<path:path>", methods=["GET"])
    def get_sha256(path: str):
        """Return the requested data"""
        parts = path.split("/")
        assert len(parts) in [1, 2]
        similar = len(parts) == 2 and parts[0] == "like"
        assert len(parts) == 1 or similar
        identifier = parts[-1]
        assert len(identifier) == IDENTIFIER_SIZE
        data_dir = os.path.join(storage_path, identifier[:3])

        if not similar:
            data_path = os.path.join(data_dir, identifier[3:])

            if not os.path.isfile(data_path):
                return "Data not currently on node", 504

            return flask.send_file(data_path, DATA_MIMETYPE)

        if not os.path.isdir(data_dir):
            return "{}", 404

        potential = [
            identifier[:3] + n
            for n in os.listdir(data_dir)
            if len(n) == IDENTIFIER_SIZE - 3
        ]
        potential.sort(
            key=lambda i: identifier_match_score(i, identifier), reverse=True
        )
        body = json.dumps(
            {
                f"/sha256/{i}": os.path.getsize(os.path.join(data_dir, i[3:]))
                for i in potential[:MAX_LIKE]
            }
        )
        response = flask.Response(body)
        response.status = 200 if len(potential) > 0 else 404
        return response

    @app.route("/sha256/<identifier>", methods=["PUT"])
    def put_sha256(identifier: str):
        """Return the requested data"""
        assert len(identifier) == IDENTIFIER_SIZE
        data_path = os.path.join(storage_path, identifier[:3], identifier[3:])

        body = "data received"
        response = flask.Response(body)
        os.makedirs(os.path.split(data_path)[0], exist_ok=True)

        with open(data_path, "wb") as block_file:
            block_file.write(flask.request.data)

        response.status = 200
        return response

    return app


def serve(args):  # NOT TESTED - no running server during tests
    """Start the libernet web server"""
    log_path = os.path.join(args.storage, "log.txt")
    os.makedirs(os.path.split(log_path)[0], exist_ok=True)
    log_level = logging.DEBUG if args.debug else logging.WARNING
    logging.basicConfig(filename=log_path, level=log_level)
    app = create_app(args)
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
