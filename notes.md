
Table of Contents
=================

* [Table of Contents](#table-of-contents)
* [Definitions](#definitions)
* [Concepts](#concepts)
  * [Data Identity](#data-identity)
  * [Data Routing](#data-routing)
  * [Deleting Data](#deleting-data)
  * [Data Matching](#data-matching)
    * [Cost to match digits](#cost-to-match-digits)
* [Data Types](#data-types)
  * [Small file](#small-file)
  * [Large file](#large-file)
  * [Bundle Description](#bundle-description)
  * [Address History](#address-history)
    * [Resolving web addresses](#resolving-web-addresses)
  * [Personal Key](#personal-key)
    * [Personal Information](#personal-information)
  * [Messages](#messages)
    * [Message Dictionary](#message-dictionary)
    * [Carrier Dictionary](#carrier-dictionary)
  * [Trust](#trust)
    * [Direct Trust](#direct-trust)
    * [Domain Trust](#domain-trust)
    * [Indirect Trust](#indirect-trust)
    * [Trust Document](#trust-document)


# Definitions

* *Hash* - SHA256 is used.
* *Encrypt* - Typically refers to AES256 encryption using the contents Hash (SHA256) as the key.
* *Public/Private Key* - An RSA public/private key pair
* *Public-key encrypted* - RSA public key used to encrypt data so only the paired RSA private key can decrypt.
* *Signing* - RSA private encryption of SHA256 hashing of the data, stored as hex.
* *Compression* - zlib compression, level 9


# Concepts


## Data Identity

Data is identified by the hash of the contents.
Any data block can, therefore, be validated that the identity is correct by hashing the data block and verifying it matches the stated identity.


## Data Routing

When new data is created, it is pushed (at the least) to the connected node that most closely matches the identity of the data.
When data is pushed to a node, it must either be stored, or pushed to another node.
A node may not delete data (except for the cases in [Deleting Data](#deleting-data)) without first passing the data on to another node.
When data is passed on before deleting, it is passed to the node that most closely matches the identity of the data.
When a node originates data, it may want to push the data to more than one node, to ensure the data is seeded properly, before deleting locally.


## Deleting Data

Certain data can be squelched (not passed on and deleted from local cache).
Each node keeps a list of identifiers that are squelched (deleted).
When data of a squelched identifier is pushed to the node, it can immediately delete it.
When responding to ["similar to" search](#data-matching), squelched data should be excluded.

Reasons for squelching data include:
* Old, fully Merged [Address History](#address-history) (assuming no history is lost)
* Old [Personal Information](#personal-information) (either through owner updates or through merging verifiers)
* Old [Trust](#trust) lists (determine by timestamp)


## Data Matching

Data can be matched to other data by adding random data until the first hex digits of the hashes match.
Data can be requested by exact hash, or by "similar to" search.
The more digits that match, the more time (in general) and expense and, therefore, the higher the priority or value of the match.
When searching ["similar to" search](#data-matching) each node may drop lower priority items if the number of items being returned is "large".
Items are not dropped just because they are low priority, but because there are "too many" items to return, in which case lower priority items may be omitted.


### Cost to match digits

Below are the times to calculate a hash of sequential integer strings until it matches the hash of an empty string as well as a string of the letters a, b, c, and d.
Absolute times are somewhat irrelevant and more to give scale.
Different processors will drastically change the absolute times.
Times are provided to give relative speed of matching the number of digits.

Numbers in the table are not exact, but generally the magnitude.
Theoretical assumes that 2^(digits x 4) unique patterns will be required to ensure that many hex digits will match.
That is not necessarily the case (see hex digits = 7).

Hex Digits | Low Time   | High Time | Low Tries   | High Tries  | Category      | Theoretical Tries | Theoretical Time
---------- | ---------- | --------- | ----------- | ----------- | ------------- | ----------------- | ----------------
4          | 0.01       | 0.10      | 5,000       | 7,000       | bulk mail     | 65,536            | 0.01 seconds
5          | 0.31       | 3.32      | 200,688     | 2,148,794   | standard rate | 1,048,576         | 0.18 seconds
6          | 10.9       | 24.0      | 7,156,184   | 15,579,804  | priority mail | 16,777,220        | 2.8 seconds
7          | 11.8       | 1,449     | 70,179,856  | 938,827,609 | urgent        | 268,435,500       | 45 seconds
8          | 300        | 300       | 188,138,959 | 188,138,959 | urgent ...    | 4,294,967,000     | 12 minutes


# Data Types


Type                                          | Encrypted | Contents | [Match](#data-matching)
-----
[Small File](#small-file)                     | Yes       | binary   | None
[Large File](#large-file)                     | Yes       | json     | None
[Bundle](#bundle-description)                 | Yes       | json     | None
[Address History](#address-history)           | No        | json     | web:lowercase/path/to/bundle
[Personal Key](#personal-key)                 | No        | PEM      | None
[Personal Information](#personal-information) | No        | json     | info:personal key identifier

## Small file

Small file is a file that is 1 MiB or smaller.
The data hashed to generate the encryption/decryption key.
Then the data is compressed if compression can reduce the size.
The key previously generated from the hash of the contents is used to encrypt the data.
The data is then hashed to generate the identifier of the data.

When data is received, it is received with the [identifier](#data-identity).
If the recipient is intended to access the contents the key is also passed with the identifier.
Data can be requested by the exact identifier.
The data is then passed from node to node without being able to read the contents.

When data is received and we have the decryption key, we can get the original data.
The data is first verified that it matches the [identifier](#data-identity).
We then decrypt the data using the shared decryption key.
If the decryption key matches the hash of the decrypted contents, then we're done.
Otherwise, decompress the data.
If the data does not match the decrypted contents, then we cannot use the data is it was a bad key or there is a hash collision on the original data.


## Large file

Large files are any file that is larger than 1 MiB.
The file is broken up into up to 1 MiB chunks with each chunk being treated as a [Small file](#small-file).
Maximum number of chunks is at least 6,095 as large file description is also limited to 1 MiB in size.
This means the file limit is no lower than 6 GiB.
The file description is a json array of chunk identifiers.
The first element of the array describes the overall file contents.
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
This json is treated like a [small file](#small-file).


## Bundle Description

A bundle description describes a bundle of files.
Each entry is around 512 bytes, so limit of files in a bundle is roughly no less than 2,000 files.
A bundle description is a json dictionary of relative path within the bundle to information about each file.
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
The *size* fields can be used to help determine if the file is stored as a [small file](#small-file) or a [large file](#large-file).
When restoring, files larger than 1 MiB are assumed to be a [large file](#large-file), but if it doesn't result in [large file](#large-file) json, then try again treating it as a [small file](#small-file).
It may be that the file compressed down below 1 MiB.
If it is smaller tha 1 MiB, it should be a [small file](#small-file).


## Address History

Address history describes current [bundle](#bundle-description) and lists previous [bundles](#bundle-description) for a web address.
Each entry is at least 330 bytes, histories can be up to 3,000 entries.
Older entries are removed to get the size down to 1 MiB.
Address histories are not encrypted, but may be compressed if compression improves size.
The identifier of an address history is a hash of compressed contents (or uncompressed if uncompressed is smaller).
Unlike a [Small file](#small-file), address history is not encrypted.

Address history is requested for a particular address via [Data Matching](#data-matching).
The path to the contents referenced by the history is first converted to lowercase.
Then any backslashes (\\) are converted to slashes (/).
Any slash prefixes are removed and then whitespace is removed from the ends.
The path is then prefixed with "web:".
```\Testing\the\Path\To Enlightenment -> web:testing/the/path/to enlightenment```
This string is then hashed.
This is the hash we will [match](#data-matching) when adding padding.

Address history is requested using ["similar to" search](#data-matching).
Many versions are likely to be returned.
All versions returned are evaluated.
A list of [bundles](#bundle-description) is built up and the address history that lists all recent [bundles](#bundle-description) is chosen.
If there is not one address history that contains all recent [bundles](#bundle-description), then a new address history is created.

When merging address histories to get the complete list of recent [bundles](#bundle-description), the head is chosen using your [trust](#trust) network.
The head is chosen from the heads in the address histories that are being merged.

Any address histories that that do not add any new [bundles](#bundle-description) outside of the chosen address history may be [deleted](#deleting-data).
A similar method is used to [delete](#deleting-data) older address histories
 and maintain the smallest number of older address histories to maintain the smallest set possible of address histories that contain all bundles.

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


### Resolving web addresses

When a web address is received, it is really an address history address and a relative path in the bundle.
Since there is not a clear distinction of where the relative path starts, there will need to be some searches.
Start looking for the bundle starting at root (hash of "web:").
If the relative path does not exist in the trusted root bundle then add the first path element (ie web:apple.com, web:company, web:country, etc).
To prevent link-hijacking and cut down on requests needed to resolve paths, bundles should be stored in the first two path items.


## Personal Key

A personal key is a PEM encoded public key that is stored without encryption (similar to [Address History](#address-history)) but with possible compression if it reduces the size.
The [identity](#data-identity) of the personal key is the identifier for a person.

### Personal Information

Personal information about a personal key is stored in a json dictionary.
This is also stored without encryption but can be compressed to reduce size.
The padding is added to [match](#data-matching) the personal key [identity](#data-identity) with "info:personal key identifier".

Personal information is requested using ["similar to" search](#data-matching).
When multiple are found, the one with the latest timestamp is used.
Older personal information should be [deleted](#deleting-data) if all or most verifiers have verified the new data and *next* field has not changed.
Multiple versions of the same timestamp can exist with different sets of verifiers.
The personal information that contains all verifiers of the latest personal information should be kept and others should be [deleted](#deleting-data).
If there is not a personal information that contains all verifiers, you may merge verifiers and republish the personal information.
If you do create a new personal information, you should [delete](#deleting-data) all other personal information.
You do not have to verify the person to merge verifiers.

Verifiers are people who can vouch for the personal information.
They may have had communication with this identity, or may be the next, or backup, identity.
They may even provide a services where they privately receive information (like scans of drivers licenses, passport, etc)
 from the identity and confirm they are who they say they are.

Personal information fields are mostly optional.
Nickname is required as it is the display name.
Credentials may not be a good idea as identification may be personal information that should not be shared.
Credentials and verifiers are two ways to help people know that this person is who they say they are.
Information that you may not want to share are email, apartment, street number (or even street or city), first name, last name.
Whenever personal information is updated, verifiers should be [messaged](#messages) to notify them of the change to give them an opportunity to verify.

Whenever personal information is created, a backup (*next*) personal key should be created.
The next field is this backup key.
In the event that your private key for the personal key is leaked, you can set the valid field to false.
Anything signed after the timestamp when the personal information is marked *valid=false* should be considered not signed.
[Trust](#trust) trees should be updated with this timestamp and anything after this timestamp should not be trusted and treated as malevolent.

If *valid=false* then the *next* value is considered an alias for valid signatures after timestamp.
The *next* fields should be generated and its private key should be put in cold storage and not used until *valid=false*.
The *next* should not be trusted in the same personal information in which *valid=false*.
Previous personal information should be sought out to see other personal keys for *next*.
The oldest *next* with personal information should be used.
When personal information is invalidated via *valid=false*, the preferred (oldest viable) *next* should be activated by generating personal information for that identity.


```
{
	"nickname": short name or handle for person,
	"first name": name,
	"last name": name,
	"domain": a domain name that http serves a file named libernet.trust at the root,
	"email": email,
	"website": address of a website,
	"twitter": twitter handle,
	"facebook": facebook url,
	"youtube": youtube url,
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
	"valid": true or false if false then this key cannot be used,
	"next": personal information identifier for a backup identity
}
```
The above dictionary is stored in the following wrapper
```
{
	"identity": above dictionary in a string,
	"signature": signature data of identity string from signer,
	"signer": hash of public key,
	"padding": random data to get hash of data to match digits with public key hash,
	"verifiers": {identifier of public key: signature of identity}
}
````


## Messages

Messages are json files that describe from, to, cc, subject, and [bundle](#bundle-description) of message body.
Messages are composed of two separate data blocks, the envelope and the carrier.
The envelope is treated as a [small file](#small-file) and contains sender's signature, routing and general message headers.
The carrier is stored unencrypted, but may be compressed.
Messages are sent by [matching](#data-matching) the message with the hash of "message:YYYY:MM:DD:public key identifier" in all lowercase.

Messages are requested using ["similar to" search](#data-matching).
They are requested for specific dates (in GMT).

The encryption key to decrypt the message is encrypted using the recipients public key.


### Message Dictionary
```
{
	"from": identifier of personal key of sender,
	"to": [list of identifiers of personal keys],
	"cc": [list of identifiers of personal keys],
	"subject": subject of the message,
	"date": YYYY/MMM/DD HH:MM:SS in GMT,
	"reply": identifier of the message being replied to,
	"sha256": the identifier of the body Bundle,
	"aes256": the key to decrypt the body Bundle,
}
```
The above dictionary is encoded into the following Envelope Dictionary
```
{
	"message": the above dictionary,
	"signature": signature of message from signer,
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

Each person collects trust into a trust list.
Trust lists are shared by [matching](#data-matching) to: "trust:public key identifier".
Trust lists are compressed (if it makes it smaller) but are not encrypted.
The shared trust list does not represent the trust tree, just the first-order, direct trusts.
When using trust lists, the most recent trust list for each trusted identity is loaded.
This can be done recursively (loading more and more trust lists) depending on the trust of each identity.

If an identity is found in your first-order, direct trust list, that is the trust value assigned to an identity.
If an identity is not found in your first-order, direct trust list, then the domain trust is checked.
If the identity is not in the domain trust list, then second-order trust lists are used (then third-order, and so on).

Temporary trust can be given to "try out" something, to see if it is what is advertised and be able to mark as trusted.
For websites and apps (special websites) this would allow downloading the bundle and being able to manually inspect it before trusting it.

For web display, a node can specify whitelist, blacklist, or graylist mode.
Whitelist mode only allows trusted (positive score) identities.
Blacklist mode only disallows untrusted (negative score) identities.
Graylist mode allows whitelist and any other identity that is not blacklisted (unknowns).

For [messages](#messages), senders are evaluated via this trust method.
Trusted senders go to the inbox.
Graylist senders (unable to determine a trust score) to to the spam folder.
Blacklist senders go to the trash.


### Direct Trust
Is it in the top level trust (we trusted or distrusted the person explicitly):

Field      | Points Scaler
---------- | -------------
trusted    | x +10
mistaken   | x -1
disagree   | x -5
malevolent | x -100


### Domain Trust

If there is no first-order, direct trust information about an identity and there is a domain context available, we check for domain trust.
If the [address](#address-history) starts with a domain name, for example apple.com, check the domain for identities.
An http request on the root of the domain for /libernet.trust file.
The libernet.trust text file just contains a list of the [identity](#data-identity) of the [personal keys](#personal-key) that are trusted by the domain, with one identity per line.
For [messages](#messages), if the [personal information](#personal-information) contains a domain, this will be used.

Domain trust may be moved below second-order, indirect trust to handle websites that have objectionable content.


### Indirect Trust

If the identity is not found in the first-order, direct trust list and not found in domain trust, we use indirect trust.
We walk through all first level identities whose score is greater than zero and ask:
* Is it in their trust list?
* Add up the scores (some positive, some negative) from each second-order, trusted identity.

If no second-order, indirect trust can be established, we move on to third-order, then fourth-order, and so on.
For the first level identities whose score is greater than zero but does not list the identity, walk through those looking for scores greater than zero that list the identity.

We go through each level until we find a level that contains the identity for which we are seeking trust information.
If we don't find any trust information, we then look at the identity's trust list and see how much we agree with his trusted identities.


### Trust Document

The trust document is [matched](#data-matching) with "trust:trust owner's public key identifier"
```
{
	"timestamp": seconds since epoch,
	"trust": {
		public key identifier: {
			"trusted": count of times content has been marked as trusted,
			"mistaken": count of times content was marked mistaken (honest mistake suspected)
			"disagree": count of times content was marked as disagree (maybe not correct, or maybe correct, but dislike presenation or stance)
			"malevolent": count of times contents was marked as malevolent intent
			"timestamp": optional field that if set marks that trust information is only valid until this timestamp
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
	"padding": random data to get hash of data to match,
}
```

