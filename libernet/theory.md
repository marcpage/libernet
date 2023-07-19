# Theory behind LiberNet

1. Data can be stored and retrieved by using the hash of the contents as the identifier
1. Data can be encrypted using the hash of the contents as the encryption key
1. With the hash of the contents (encrypted or unencrypted) you can get the data
1. Data encrypted with the contents hash can be verified as being correct
1. Data can also be encrypted using the hash of a pass phrase (no verification of contents)
1. Random data can be appended to data to make its hash "close to" another hash
1. "Close to" hashes can be hashes of usernames, dates, etc
1. This allows us to store data with unknown contents for later retrieval
1. It may be beneficial to compress the original data (but rarely the encrypted data)

## Data stored via hashes of contents

Every block of data is stored with the hash of its contents.
This allows us to retrieve known data by simply keeping track of the contents hash.
It also allows us to verify that the block of data is correct for the name.

We use sha256, which means that we might have a natural collision 1 in 10<sup>38</sup>
blocks.
With 1 MiB blocks, that means we would likely not see 1 collision until we've reached 9
billion billion exabytes of data stored.
In 2011 it was estimated that the world data storage capacity was around 300 exabytes.
[ZDNet](https://www.zdnet.com/article/what-is-the-worlds-data-storage-capacity/)


## Encrypting with the contents as the key

We can use the hash of the contents of the data as an encryption key.
We would then store the data with the hash of the encrypted data.
To retrieve the data we would use the hash of the encrypted data.
To fully access the original data we would need the hash of the encrypted data and the
hash of the original data.
This also allows us to verify that the unencrypted data is correct.

We use AES256 for encryption and use the SHA256 hash of the contents as the key.

## Encrypting with a pass phrase

Instead of encrypting with the hash of the contents, we could encrypt with the hash of a
pass phrase.
Again we would just need the hash of the encrypted data to retrieve the encrypted data.
To decrypt we would need the pass phrase.
This would allow for human identity verification.
The downside is that the final unencrypted data cannot be verified to be correct.

We use AES256 for encryption and use the SHA256 hash of the pass phrase as the key.


## Storing data "close to" other data

We can add random data to a block (encrypted or not) to adjust the final hash.
This can be done repeatedly until the final storage id hash is "close to" known hash.
The more digits in the hash match the known hash the closer they are and likely more CPU
power needed to compute.

This allows us to send data to a known location to be retrieved later.
The known hash could be a username, a date, an operation, or any combination.
Since random data may also happen to be "close to" the known hash, the "close" data blocks
will need to be vetted.
This also means that requesting data "close to" a known hash may be limited due to shear
number of blocks.
If newer data supersedes older data, it should be "closer to" the known hash to ensure it
will be seen.
If the number of blocks being stored "close to" the known hash is large, consider
sequencing the hash (like including year, month, day, hour, and/or minute).

If data at the known location is encrypted, it should be encrypted with a password.
If encrypted with a password then the contents will need a way to be verified or ensure
the contents are sane (known format).

As an example, messages could be sent to user `John` by hashing `MESSAGES: 2023/07/31 John`.
The messages would be stored near that hash.
When John want's to retrieve his messages, he just gets data "close to" the hash of
`MESSAGES: 2023/07/31 John`.
In this example all of John's messages are completely publicly available unless there is a
shared pass phrase between the sender and John.

We require at least 12 leading bits (3 hex digits) of the hash to match, limit the number
matching block identifiers returned to 100, and add a null byte followed by random
non-null bytes to achieve the match.


## Compression

Often data being stored can benefit from being compressed.
Compression should be done after the hash of the contents is calculated.

For unencrypted data, the identifier of the block is the hash of the original contents.
Thus to verify a block, you must check it against the hash of the data and then against
the hash of the decompressed data.

For encrypted data, the data block identifier is **always** the hash of the block.
The encryption key is the hash of the original contents.
If the hash of the decrypted data matches the encryption key, the data is not compressed.
If they do not match, decompress the data and and verify the hash of the decompressed
data matches the encryption key.

Data that is encrypted with a pass phrase should **not** be compressed since you cannot
verify the final contents.
