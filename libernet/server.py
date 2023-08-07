#!/usr/bin/env python3

"""
    Libernet Server
"""

import os
import logging
import argparse
import json
import time
import zipfile

from zipfile import ZipFile

import flask

import libernet.url
import libernet.message

from libernet.disk import Storage
from libernet.url import SHA256, LIKE

DATA_MIMETYPE = "application/octet-stream"
SETTINGS_NAME = "settings.json"
JSON_MIMETYPE = "application/json"
DEFAULT_PORT = 8042
DEFAULT_STORAGE = os.path.join(os.environ["HOME"], ".libernet")
ONE_GIGABYTE = 1024 * 1024 * 1024


def create_app(storage: Storage, messages: libernet.message.Center):
    """Creates the Flask app"""
    app = flask.Flask(__name__)

    @app.route(f"/{SHA256}/<path:path>", methods=["GET"])
    def get_sha256(path: str):
        """Return the requested data"""
        assert path in flask.request.path, f"{path} vs {flask.request.path}"
        identifier, key, _, kind = libernet.url.parse(flask.request.path)
        assert key is None, flask.request.path
        data_block_url = libernet.url.for_data_block(identifier, kind == LIKE)
        node_identifier = None  # TODO: add node identifier  # pylint: disable=fixme
        node_inet_address = None  # TODO: add inet address  # pylint: disable=fixme

        if kind == LIKE:
            found = storage.like(data_block_url)
            response = flask.Response(json.dumps(found), mimetype=JSON_MIMETYPE)
            response.status = 200 if len(found) > 0 else 404
            messages.send(
                {
                    "type": "request",
                    "style": "like",
                    "method": SHA256,
                    "identifier": identifier,
                    "found": len(found),
                    "node": node_identifier,
                    "address": node_inet_address,
                }
            )
            return response

        contents = storage.get(data_block_url)

        if contents is None:
            messages.send(
                {
                    "type": "request",
                    "style": "data",
                    "method": SHA256,
                    "identifier": identifier,
                    "found": False,
                    "node": node_identifier,
                    "address": node_inet_address,
                }
            )
            return "Data not currently on node", 504

        response = flask.Response(contents, mimetype=DATA_MIMETYPE)
        response.status = 200
        messages.send(
            {
                "type": "request",
                "style": "data",
                "method": SHA256,
                "identifier": identifier,
                "found": True,
                "node": node_identifier,
                "address": node_inet_address,
            }
        )
        return response

    @app.route(f"/{SHA256}/<identifier>", methods=["PUT"])
    def put_sha256(identifier: str):
        """Return the requested data"""
        storage[libernet.url.for_data_block(identifier)] = flask.request.data
        node_identifier = None  # TODO: add node identifier  # pylint: disable=fixme
        node_inet_address = None  # TODO: add inet address  # pylint: disable=fixme
        messages.send(
            {
                "type": "provide",
                "style": "data",
                "method": SHA256,
                "identifier": identifier,
                "valid": True,  # TODO: Validate the data  # pylint: disable=fixme
                "node": node_identifier,
                "address": node_inet_address,
            }
        )
        return "data received", 200

    return app


def rotate(path):
    """zips up any existing file and then deletes the file if it exists"""
    if not os.path.isfile(path):
        os.makedirs(os.path.split(path)[0], exist_ok=True)
        return None

    file_time = os.path.getmtime(path)
    zip_path = f"{path}_{time.strftime('%Y-%b', time.localtime(file_time))}.zip"
    name, extension = os.path.splitext(os.path.basename(path))
    file_time_string = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(file_time))
    name_in_archive = f"{name}_{file_time_string}{extension}"

    with ZipFile(zip_path, "a") as zip_file:
        zip_file.write(path, name_in_archive, zipfile.ZIP_DEFLATED, 9)

    os.remove(path)
    return zip_path


def serve(args):  # NOT TESTED (not reported as tested, tested in tests/test_proxy.py)
    """Start the libernet web server"""
    log_path = os.path.join(args.storage, "log.txt")
    rotate(log_path)
    log_level = logging.DEBUG if args.debug else logging.WARNING
    logging.basicConfig(filename=log_path, level=log_level)
    storage = Storage(args.storage)
    messages = libernet.message.Center()
    libernet.message.Logger(messages)
    app = create_app(storage, messages)
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
