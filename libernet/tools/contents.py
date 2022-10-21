#!/usr/bin/env python3

""" Libernet content to serve
"""

import logging
import os
import io
import zlib

import libernet.plat.network
import libernet.tools.block


def forbidden():
    """Return response for forbidden content"""
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


def home(remote_addr: str, environ):
    """
    remote_addr - flask.request.remote_addr
    environ - flask.request.environ
    http://localhost:8000/?

    environ['REQUEST_URI'] = '/?',
    environ['RAW_URI'] = '/?',
    """
    if libernet.plat.network.is_on_machine(remote_addr):
        return f"""
<html>
    <body>
        <h1>Welcome</h1>
        {remote_addr}
        <br/>
        REQUEST_URI = '{environ['REQUEST_URI']}'<br/>
        RAW_URI = '{environ['RAW_URI']}'<br/>
    </body>
</html>"""
    return forbidden()


# pylint: disable=R0911
def sha256(path, remote_addr, storage):
    """
    remote_addr - flask.request.remote_addr
    storage - settings.storage()
    """
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
    local_request = libernet.plat.network.is_on_machine(remote_addr)

    if block_key is not None and not local_request:
        logging.debug("block_key is not None and not local_request")
        return forbidden()

    if path_in_bundle is not None:
        bundle = libernet.tools.bundle.Path(full_url, storage)

        if path_in_bundle == "":
            path_in_bundle = bundle.index()

            if path_in_bundle is None:
                return (
                    f"<html><body>{full_url} not found, bundle has no index</body></html>",
                    404,
                )  # File not found

        bundle_path = os.path.join(
            storage,
            libernet.tools.block.WEB_SUBDIR,
            bundle.relative_path(just_bundle=True),
        )
        item_path = os.path.join(bundle_path, path_in_bundle)
        already_exists = os.path.isfile(item_path)
        missing = bundle.missing_blocks(path_in_bundle) if not already_exists else []
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
            return ("file", item_path, {})

        contents = None  # need to get full contents of file then send file

    else:
        try:
            contents = libernet.tools.block.get_contents(
                storage, block_identifier, block_key
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
    return (
        "file",
        content_file,
        {
            "mimetype": "application/octet-stream",
            "as_attachment": True,
            "max_age": 365 * 24 * 60 * 60,
            "download_name": block_identifier if block_key is None else block_key,
            "etag": block_identifier if block_key is None else block_key,
        },
    )
