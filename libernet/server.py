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
SETTINGS_NAME = "settings.json"
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


def serve(args):  # NOT TESTED (not reported as tested, tested in tests/test_proxy.py)
    """Start the libernet web server"""
    log_path = os.path.join(args.storage, "log.txt")
    os.makedirs(os.path.split(log_path)[0], exist_ok=True)
    log_level = logging.DEBUG if args.debug else logging.WARNING
    logging.basicConfig(filename=log_path, level=log_level)
    storage = Storage(args.storage)
    app = create_app(storage)
    app.run(host="0.0.0.0", debug=args.debug, port=args.port)


def load_settings_file(args, name):
    """Load settings file with the given filename"""
    os.makedirs(args.storage, exist_ok=True)
    settings_path = os.path.join(args.storage, name)
    try:
        with open(settings_path, "r", encoding="utf-8") as settings_file:
            return json.load(settings_file)

    except FileNotFoundError:
        return {}


def save_settings_file(args, name, settings):
    """Save settings file with the given filename"""
    settings_path = os.path.join(args.storage, name)

    with open(settings_path, "w", encoding="utf-8") as settings_file:
        json.dump(settings, settings_file, indent=2)


# pylint: disable=too-many-arguments
def check_arg(value, key, default, value_type, prompt, settings, input_func):
    """Check an argument against settings"""
    if value is None and settings.get(key, None) is None:
        value = value_type(input_func(prompt)) if default is None else default

    if value is None:
        value = settings.get(key, default)

    elif value and settings.get(key, None) != value:
        settings[key] = value
        return True, value

    return False, value


def load_settings(args, input_func=input):
    """Update settings file from arguments and arguments from settings"""
    settings = load_settings_file(args, SETTINGS_NAME)
    save_settings, args.port = check_arg(
        args.port,
        "port",
        DEFAULT_PORT,
        int,
        "Port to listen on (--port): ",
        settings,
        input_func,
    )

    if save_settings:
        save_settings_file(args, SETTINGS_NAME, settings)

    return args


def get_arg_parser():
    """Describe the command line arguments"""
    parser = argparse.ArgumentParser(description="Libernet server")
    parser.add_argument(
        "-p",
        "--port",
        type=int,
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
    serve(load_settings(get_arg_parser().parse_args()))  # NOT TESTED
