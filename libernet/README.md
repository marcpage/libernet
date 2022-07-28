# Libernet tools

Libernet is composed of a few tools.
You will need to `pip3 install -r requirements.txt` to run the tools.


## bundle

The bundle tool allows you to create bundles and restore them.
A bundle can be used as a backup or can be a libernet webpage.

You can bundle a directory via:

```python3 -m libernet.bundle --dir {directory} store```

A path will be returned. This path will be required to restore.

```python3 -m livernet.bundle --dir {destination} --url {path returned} restore```



## server

The server can be run via:

```python3 -m libernet.server```

It starts a web server on, by default, port 8000.

You can access files inside bundles by using the path returned from bundle.

```http://localhost:8000{path returned}```

This would look something like:

```http://localhost:8000/sha256/4ed222c87d82dfbd2d97c4d3c21507acf0f4aa0276d6a36b7d520c54478abb0a/aes256/83a3605e47aa56d70cc263a01dc323b9b9472a43c820d576f642e1671dbfe462/index.html```

You can set a starting point file by passing `--index`

```python3 -m libernet.bundle --dir {directory} --index index.html store```

Which would make the above URL just (note the trailing slash (/)):

```http://localhost:8000/sha256/4ed222c87d82dfbd2d97c4d3c21507acf0f4aa0276d6a36b7d520c54478abb0a/aes256/83a3605e47aa56d70cc263a01dc323b9b9472a43c820d576f642e1671dbfe462/```

