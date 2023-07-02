# Libernet tools

Libernet is composed of a few tools.
You will need to `pip3 install -r requirements.txt` to run the tools.


## server

The server can be run via:

```python3 -m libernet.server```

It starts a web server on, by default, port 8042.

You can access files inside bundles by using the path returned from bundle.

```http://localhost:8042/sha256/{data identifier}```

This would look something like:

```http://localhost:8000/sha256/4ed222c87d82dfbd2d97c4d3c21507acf0f4aa0276d6a36b7d520c54478abb0a```

GET and PUT requests to data are supported.
