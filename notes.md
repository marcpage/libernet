
Table of Contents
=================

* [Concepts](#concepts)
  * [Data Routing](#data-routing)
  * [Deleting Data](#deleting-chunks)
  * [Data Addresses](#data-addresses)
    * [Time to match digits](#time-to-match-digits)
* [Data Types](#data-types)
  * [Small file](#small-file)
  * [Large file](#large-file)
  * [Bundle Description](#bundle-description)
  * [Address History](#address-history)
  * [Personal Key](#personal-key)
  * [Messages](#messages)
    * [Message Dictionary](#message-dictionary)
    * [Carrier Dictionary](#carrier-dictionary)
  * [Trust](#trust)
    * [Direct Trust](#direct-trust)
    * [Domain Trust](#domain-trust)
    * [Indirect Trust](#indirect-trust)

# Concepts


## Data Routing

When new data is created, it is pushed (at the least) to the connected node that most closely matches the identity of the data
Data identity is the hash of the contents of the data
When data is pushed to a node, it must either be stored, or pushed to another node
A node may not delete data (except for the cases in Deleting Chunks) without first passing the data on to another node
When data is passed on before deleting, it is passed to the node that most closely matches the identity of the data
When a node originates data, it may want to push the data to more than one node, to ensure the data is seeded properly, before deleting locally


## Deleting Data

Certain data can be squelched (not passed on and deleted from local cache)
Each note keeps a list of identifiers that are squelched (deleted)
when data of a squelched identifier is pushed to the node, it can immediately delete it
These include
* Old, fully Merged Address History (assuming no history was lost)
* Old Personal Key Identity files (either through owner updates or through merging verifiers)
* Old trust lists (determine by timestamp)


## Data Addresses

Data is addressed by the hash of the contents (which may be compressed or encrypted or both)
Data can be matched to other data by adding random data until the first hex digits of the hashes match
Data can be requested by exact hash, or by "similar to"
The more digits that match, the more time (in general) and expense and the higher the priority
When searching "similar to" each node may drop lower priority items if the number of items being returned is "large"

### Time to match digits
Digits | Low Time   | High Time | Low Tries   | High Tries  | Category      | Theoretical Tries | Theoretical Time
------ | ---------- | --------- | ----------- | ----------- | ------------- | ----------------- | ----------------
4      | 0.01       | 0.10      | 5,000       | 7,000       | bulk mail     | 65,536            | 0.01 seconds
5      | 0.31       | 3.32      | 200,688     | 2,148,794   | standard rate | 1,048,576         | 0.18 seconds
6      | 10.9       | 24.0      | 7,156,184   | 15,579,804  | priority mail | 16,777,220        | 2.8 seconds
7      | 11.8       | 1,449     | 70,179,856  | 938,827,609 | urgent        | 268,435,500       | 45 seconds
8      | 300        | 300       | 188,138,959 | 188,138,959 | urgent ...    | 4,294,967,000     | 12 minutes


# Data Types


## Small file

data compressed (if compression actually decreases size)
data hashed for aes key
data encrypted
data hashed for identifier

data received from identifier
data decrypted using shared secrete aes key/contents hash
data hashed, if matches hash, we're done
data decompressed
data hashed, if not matches aes key, then there was an error


## Large file

File broken in up to 1 MiB chunks and each chunk is treated like a [Small file](#small-file)
Maximum number of chunks is at least 6,095 (assuming no compression)
This means the file limit is no lower than 6 GiB (with compression, it is likely to be higher)
json file of array of chunk identifiers. First element describes the overall file contents
```
	[
		{
			'sha256': entire file hash,
			'size': data size
		},
		{
			'sha256': data identifier,
			'aes256': decryption key,
			'size': data size
		}
	]
```
This json file is treated like a [Small file](#small-file)


## Bundle Description

Describes a bundle of files
Each entry is around 512 bytes, so bundle file limit is no less than 2,048
json file of dictionary of relative path within the bundle to information about the file
```
{
	"index.html": {
		"sha256": identifier of Small or Large file,
		"aes256": key to decrypt Large file description or small file,
		"size": number of bytes in the final file,
		"Content-Type": mime type for the data},
	"images/logo.png": {
		"sha256": identifier of Small or Large file,
		"aes256": key to decrypt Large file description or small file,
		"size": number of bytes in the final file,
		"Content-Type": mime type for the data},
}
```


## Address History

Describes current bundle and previous bundles.
Each entry is at least 330 bytes, histories can be up to 3,000 entries
Older entries are removed to get the size down to 1 MiB
These files are not encrypted, but may be compressed if compression improves size
Identifier is hash of compressed contents (or uncompressed if uncompressed is small)
When multiple bundles are found, they are merged and a new bundle is published
Merging is really just looking for list of all unique bundles, trimming the list down to fit in 1 MiB
Choosing the head is a matter of trust, number of signatures, timestamp, agreement, and possibly user input.
'padding' field is chosen at random until the first N hex digits of the hash of the history matches the same digits in the hash of the Address
To hash the address, convert the path to lowercase, convert backslash (\) to forward slash (/), strip whitespace on the ends, and hash
\Testing\the\Path\To Enlightenment -> /testing/the/path/to enlightenment
Ultimate trust is given to the DNS owner
for /apple.com/path/to/item.html we would check http://apple.com/libernet.key for the identifier of the public key that owns this space. That key has ultimate ownership of this area
```
{
	"sha256": identifier of bundle,
	"aes256": key to decrypt bundle,
	"timestamp": seconds since epoch,
	"signed": {identifier of public key: signature of bundle identifier},
	"padding": random number to get hash to fit the right pattern,
	"previous":[
		{
			"sha256": identifier of bundle,
			"aes256": key to decrypt bundle,
			"timestamp": seconds since epoch,
			"signed": {identifier of public key: signature of bundle identifier},
		},
	]
}
```


## Personal Key

PEM encoded public key is stored without encryption (similar to Address History) but with possible compression
this hash is the user's identity
Address Matched to the key is a dictionary with information about that key
When multiple are found, the latest timestamp, valid record is kept, others are deleted
Multiple versions of the latest identity can exist with different verifiers
These can be merged and older versions that are missing verifiers can be deleted
```
{
	"nickname": short name or handle for person,
	"first name": name,
	"last name": name,
	"email address": email,
	"credentials": [
		{"sha256": identifier of image of some identification credentials,"aes256": key to decrypt credential image}
	],
	"country": country,
	"state": state,
	"province": province,
	"timestamp": seconds since the epoch used to determine latest profile,
	"city": city,
	"street": street,
	"street number": house number,
	"apartment": apartment number,
	"valid": true or false if false then this key cannot be used
}
```
The above dictionary is stored in the following wrapper
```
{
	"identity": above dictionary in a string,
	"signature": signature data,
	"signer": hash of public key,
	"padding": random data to get hash of data to match digits with public key hash,
	"verifiers": {identifier of public key: signature of identity}
}
````


## Messages

Messages are json files that describe from, to, cc, subject, and bundle of message body
The are possibly compressed (if it results in a smaller message)
They are encrypted using the public key of the sender
The padding is added until the message hash matches the hash of "message:YYYY:MM:DD:public key identifier" all lowercase
When requesting mail, dates are requested, starting with tomorrow and going back in time until you go one day past last day retrieved
There is an added cost per recipient of encrypting the key with their public key
The biggest cost is still the hashing of the Carrier Dictionary for each recipient
Carrier Dictionary is not encrypted
The Envelope Dictionary is compressed (if it is smaller) and encrypted

### Message Dictionary
```
{
	"from": identifier of personal key of sender,
	"to": [list of identifiers of personal keys],
	"cc": [list of identifiers of personal keys],
	"subject": subject of the message,
	"reply": identifier of the message being replied to,
	"sha256": the identifier of the body Bundle,
	"aes256": the key to decrypt the body Bundle,
}
```
The above dictionary is encoded into the following Envelope Dictionary
```
{
	"message": the above dictionary,
	"signature": signature message,
	"signer": hash of public key,
}
````

### Carrier Dictionary
```
{
	"sha256": the identifier of the message,
	"key": {"aes256": the public-key encrypted key to decrypt the message}
	"padding": random data to get hash of data to match digits with public key hash,
}
```


## Trust

Each person collects trust into a trust list
Trust lists are stored in an address matched to: "trust:public key identifier"
Trust lists are compressed (if it makes it smaller) but are not encrypted
They do not represent tree-trust
When using trust lists, the most recent trust list for each person trusted is loaded
This can be done recursively (loading more and more trust lists) depending on the trust of each identity
First level trust is given the highest score
Domain trust is given the next highest score
Then each level deeper in the trust tree is less and less scored

Temporary trust can be given to "try out" something, to see if it is what is advertised and be able to mark as trusted
For websites and apps (special websites) this would allow downloading the bundle and being able to manually inspect it before trusting it

For web display, a node can specify whitelist, blacklist, or graylist mode
whitelist only allows trusted identities
blacklist only disallows untrusted identities
graylist allows whitelist without question, absolutely prevents blacklist without question, and will prompt for anything that is not clearly white or black
whitelist is only good marks
blacklist is only bad marks
graylist is don't know or mixed results

email messages are done in a similar way with senders
email blocking can be done whitelist, blacklist, or graylist
Unknown senders can be evaluated, look at their identity information, etc and decide to whitelist them

To determine if an identity can be trusted:


### Direct Trust
Is it in the top level trust (we trusted or distrusted the person explicitly)

Field      | Points Scaler
---------- | -------------
trusted    | x +10
mistaken   | x -1
disagree   | x -5
malevolent | x -100


### Domain Trust
If the identity is the domain identity: 1 point


### Indirect Trust

Then walk through all first level identities whose score is greater than zero
* is it in their trust list?
* If so, the score is scaled by the ratio of identity score to the maximum identity score
* Each first-order-identity can have at most 0.1 point that it adds (0.1 point score can only be by most trusted first-level identity)

For the first level identities whose score is greater than zero but does not list the identity, walk through those looking for scores greater than zero that list the identity. These get a maximum score of 0.01 scaled to the owning identity.

And so on unto we find a level that contains the identity for which we are seeking trust information.
If we don't have a preference, then look to the domain.
If there is no domain, how would the people we trust rate them.
If the people we trust don't know about them, what about the people they trust, and so on.
Maybe at some point it is worth it to look up the identities trust list and see how it aligns with yours (if at all).
```
{
	"timestamp": seconds since epoch,
	"trust": {
		public key identifier: {
			"trusted": count of times content has been marked as trusted,
			"mistaken": count of times content was marked mistaken (honest mistake suspected)
			"disagree": count of times content was marked as disagree (maybe not correct, or maybe correct, but dislike presenation or stance)
			"malevolent": count of times contents was marked as malevolent intent
		}
	}
}
```
The above dictionary is placed in a string in the following wrapper
```
{
	"trust": string of the above dictionary,
	"signature": signature message,
	"signer": hash of public key,
}
```

