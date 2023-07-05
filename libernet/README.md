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



# protocol


## data block size

Data blocks are limited to 1 MiB (1024 * 1024 bytes).
This limitation applies to both the compressed, encrypted, and raw data.
For data larger than 1 MiB, you can use a bundle to collect blocks into larger data.


## bundled data

Files can be bundled and the parts be pushed to the server.

A bundle has the following format:
```
{
    'bundles': [
        '/sha256/4ed222c87d82dfbd2d97c4d3c21507acf0f4aa0276d6a36b7d520c54478abb0a/aes256/4ed222c87d82dfbd2d97c4d3c21507acf0f4aa0276d6a36b7d520c54478abb0a',
        '/sha256/4ed222c87d82dfbd2d97c4d3c21507acf0f4aa0276d6a36b7d520c54478abb0a/aes256/4ed222c87d82dfbd2d97c4d3c21507acf0f4aa0276d6a36b7d520c54478abb0a',
    ],
    'directories': {
        'temp': None,
        'link_dir': "temp"
    },
    'files': {
        'docs/README.md': {
            'readonly': True,
            'executable': False,
            'size': 342,
            'link': "",
            'contents': [
                {
                    'url': '/sha256/4ed222c87d82dfbd2d97c4d3c21507acf0f4aa0276d6a36b7d520c54478abb0a/aes256/4ed222c87d82dfbd2d97c4d3c21507acf0f4aa0276d6a36b7d520c54478abb0a',
                    'size': '342'
                }
            ]
        }
    }
}
```

### bundles

This are addresses of file-only bundles that are to be merged into this bundle.
This allows for bundles that exceed the 1 MiB data limit.

### directories

Directories is a list of empty directories.
All other directories are created when the files in them are created.
If there is a link to an empty directory, the contents of the link will be stored, otherwise `None`.

### files

All files in the bundle are stored with relatives paths to the files.
The file may conditionally contain `readonly` and `executable`, and if absent are considered `false`.
The `size` field is the size of the original contents.
The `contents` is a list of `url`/`size` pairs.
The `url` contains the address to get the raw data and the size of the stored final data block.
The `size` is the size of the original data.
The sum of all block `size`s should be the file `size`.
This allows for building files as pieces come in.
