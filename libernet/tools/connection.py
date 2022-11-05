#!/usr/bin/env python3

""" Libernet connection
"""

import threading
import os
import time

import requests

import libernet.plat.dirs
import libernet.tools.block
import libernet.tools.encrypt
import libernet.tools.settings
import libernet.tools.message
import libernet.tools.contents


UPDATE_STATE_INTERVAL_SECONDS = 30.0


class Connection(threading.Thread):
    """a thread that communicates to another libernet node"""

    def __init__(self, address, port, settings):
        """create a new connection"""
        self.__state = {
            "running": True,
            "url": f"http://{address}:{port}",
            "default_name": f"{address}:{port}",
            "last_check": 0,
        }
        self.__settings = settings
        self.__identity = None
        self.__session = None
        threading.Thread.__init__(self)
        self.daemon = True
        self.start()

    def __send(self, path: str, method_get=True, contents=None):
        """send a GET or PUT request to the server"""
        headers = self.__settings.sign_request(path)

        if method_get:
            return self.__session.get(f"{self.__state['url']}{path}", headers=headers)

        return self.__session.put(
            f"{self.__state['url']}{path}",
            data=contents,
            headers=headers,
        )

    def __block_path(self, identifier):
        """Get the local path for data from the remote node"""
        server_dir = os.path.join(
            self.__settings.storage(), libernet.tools.block.UPLOAD_SUBDIR
        )
        path = (
            libernet.tools.block.block_dir(server_dir, identifier, full=True) + ".raw"
        )
        os.makedirs(os.path.split(path)[0], exist_ok=True)
        return path

    def __fetch_block(self, identifier):
        """fetch a block of data from the server, cache it locally, and return the contents"""
        response = self.__send(f"/sha256/{identifier}")

        if not response.ok:
            return None

        with open(self.__block_path(identifier), "wb") as block_file:
            block_file.write(response.content)

        self.__settings.messages().send(
            {
                "action": "received",
                "block": identifier,
                "server": None
                if self.__identity is None
                else self.__identity.identifier(),
                "connection": self,
            }
        )
        return response.content

    def __create_identity(self, identifier):
        """create an identity once we have the remote identifier"""
        pem = libernet.tools.block.get_contents(self.__settings.storage(), identifier)

        if pem is None:
            pem = self.__fetch_block(identifier)

        if pem is None:
            # if they've can't give us their public key, don't talk to them
            return False

        description = {"public": pem, "identifier": identifier}
        self.__identity = libernet.tools.encrypt.RSA_Identity(description)
        return True

    def __send_block(self, identifier):
        """send a block we have"""
        contents = libernet.tools.block.get_contents(
            self.__settings.storage(), identifier
        )
        assert contents is not None, f"Cannot send block we don't have: {identifier}"
        response = self.__send(
            f"/sha256/{identifier}", method_get=False, contents=contents
        )
        # If they are not responding as we expect, don't talk to them
        self.__state["running"] = response.ok and self.__state["running"]
        self.__settings.messages().send(
            {
                "action": "sent",
                "block": identifier,
                "server": None
                if self.__identity is None
                else self.__identity.identifier(),
                "connection": self,
            }
        )
        if response.ok and self.__identity is None:
            remote_node_identifier = response.headers.get(
                libernet.tools.settings.HTTP_AUTHOR, None
            )
            created = self.__create_identity(remote_node_identifier)
            # if we cannot determinet their identity, don't talk to them
            self.__state["running"] = created and self.__state["running"]

    def __communicate_metadata(self, name):
        if self.__identity is None:
            server = self.__state["default_name"]
        else:
            server = self.__identity.identifier()

        value = self.__send(f"/server/{name}")
        assert value.ok, value.content
        parts = [
            self.__settings.storage(),
            libernet.tools.block.UPLOAD_SUBDIR,
            server,
            "server",
        ]
        os.makedirs(os.path.join(*parts), exist_ok=True)

        with open(os.path.join(*parts, f"{name}.txt"), "wb") as text_file:
            text_file.write(value.content)

        value = libernet.tools.contents.gather(
            self.__settings.storage(), f"/server/{name}.txt"
        )
        self.__send(f"/server/{name}", method_get=False, contents=value)

    def __say_hello(self):
        """do the dance with the remote connection to initiate the connection"""
        # Send our public key as an introduction
        self.__send_block(self.__settings.identity().identifier())
        self.__communicate_metadata("requests")
        self.__communicate_metadata("searches")
        self.__communicate_metadata("servers")

    def identity(self):
        """get the identity of the remote server"""
        return self.__identity

    def run(self):
        """Loop to send requests to the remote node"""
        self.__session = requests.session()
        self.__session.headers["Connection"] = "Keep-Alive"
        self.__say_hello()

        while self.__settings.messages().active() and self.__state["running"]:
            request = self.__settings.messages().receive()
            print(f"{request}")
            now = time.time()

            if now - self.__state["last_check"] > UPDATE_STATE_INTERVAL_SECONDS:
                self.__communicate_metadata("requests")
                self.__communicate_metadata("searches")
                self.__communicate_metadata("servers")
                self.__state["last_check"] = now

class Manager(threading.Thread):
    def __init__(self, settings):
        """create a new connection manager"""
        self.__connections = []
        self.__running = True
        self.__settings = settings
        threading.Thread.__init__(self)
        self.daemon = True
        self.start()

    def __read_connections(self):
        gathered = libernet.tools.contents.gather(settings.storage(), "/server/servers.txt")
        found = []

        for line in gathered.split("\n"):
            if line.strip() and ':' in line:
                server, port = line.split(':', 1)
                found.append((server, port))
        
        return found
    
    def run(self):
        """Loop to send requests to the remote node"""
        while self.__running:
            pass
        # keep a list of bad connections and filter them out
