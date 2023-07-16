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

You can also find *similar* data accessing `/sha256/like/{data identifier}`:

```http://localhost:8000/sha256/like/4ed222c87d82dfbd2d97c4d3c21507acf0f4aa0276d6a36b7d520c54478abb0a```

This will return a JSON object mapping some of the top-matching identifiers to their block size.
An identifier matches the most characters at the beginning match.


# protocol

## addresses

| type | pattern | notes |
|---|---|---|
| data block | /sha256/{hash} | hash matches either contents or zlib decompress of contents |
| contents encrypted | /sha256/{hash}/eas256/{hash} | second hash is both the aes256 key for decrypting the data block and the sha256 hash of the unencrypted contents |
| similar data blocks | /sha256/like/{hash} | Top matches (most starting digits match) |
| passphrase encrypted | /sha256/{hash}/passphrase/{hash} | second hash is sha256 hash of utf-8 encoding of the passphrase used as an aes256 key for encryption |

## 'like' blocks

You can use `/sha256/like/{hash}` to find blocks similar to another block.
This is typically used by generating the hash of some known data (say a username) and then forcing other data to have a similar hash.
For instance, say your username was `John`.
The sha256 hash of the UTF-8 encoding of `John` is `a8cfcd74832004951b4408cdb0a5dbcd8c7e52d43f7fe244bf720582e05241da`.
If we want to store information that we can retrieve from `John`, we just make other data blocks have a **similar** hash.
We add a suffix to the data block (before compression) that starts with a null byte (`\x00`) and then followed by some non-null bytes.
We keep trying different suffixes until the data we want to store with `John` has a block identifier **close** to `John`'s.
Say we append the bytes `00 A5 6F 93 12` to our data and now our data hash is `a8c79a39992f0e6d72d28534f5b508d97d73fc96322b8db153fe0f8e73dadb1e`.
Notice both `John` and our data start with `a8c`.
The more digits match, the better the match (potentially more time spent trying to find a match).
We know that the data was stored to be **like** `John` so when we get the data, we remove that last null byte (`\x00`) and all bytes after it.

In summary, we add a null byte (`\x00`) followed by random bytes to a data block (after encryption, or before compression) to **store** data at a **known** location.
These random bytes change the data block identifier, and we keep choosing new random bytes until we get **close enough** to the known identifier.
This allows is to find any given data.

An example might be storing a user's preferences.
We use our username, `John` to determine we want to get close to `a8c...`.
We encrypt our preferences with our passphrase, and then add a null byte (`\x00`) and random bytes until our preference data block starts with `a8c...`.
We then store that modified block.

When we want to get our preferences, we ask for data blocks **like** `a8c...` (`John`'s hash).
We then parse all the data blocks we get back and see which one looks like our preferences.
Since preferences may change, it might be a good idea to store the date in the preferences along with any known older preference identifiers.
That way as we go through evaluating blocks, we can start to exclude other blocks from our search (known older blocks).
We can also choose the preferences with the latest date in it.

For data that will change often (and be flooded with like blocks), you may want the original **like id** to be more dynamic.
For instance, say you are retrieving messages sent to you.
You may want to find blocks **like** the hash of `MESSAGES:John@2023/01/01`.
This will find all messages sent to John on January 1st of 2023.

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
