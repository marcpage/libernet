
![status sheild](https://img.shields.io/static/v1?label=status&message=implementing+spec&color=inactive&style=plastic)
[![commit sheild](https://img.shields.io/github/last-commit/marcpage/libernet?style=plastic)](https://github.com/marcpage/libernet/commits)
[![activity sheild](https://img.shields.io/github/commit-activity/m/marcpage/libernet?style=plastic)](https://github.com/marcpage/libernet/commits)
[![size sheild](https://img.shields.io/github/languages/code-size/marcpage/libernet?style=plastic)](https://github.com/marcpage/libernet)
[![issues sheild](https://img.shields.io/github/issues-raw/marcpage/libernet?style=plastic)](https://github.com/marcpage/libernet/issues)
[![follow sheild](https://img.shields.io/github/followers/marcpage?label=Follow&style=social)](https://github.com/marcpage?tab=followers)
[![watch sheild](https://img.shields.io/github/watchers/marcpage/libernet?label=Watch&style=social)](https://github.com/marcpage/libernet/watchers)


# Summary

Libernet is a free, open, and secure platform for having distributed backups.
It is designed to provide a robust backup solution using a community sharing disk space.
All information is securely encrypted and distributed across the Libernet.
While information loss is possible, only information that no one is requesting would be lost.
Similar to Bittorrent, information that is more popular is distributed to more nodes, making loss less likely and transfers faster.

Note: We could use ZeroConf for local node discovery


# Platform Support

macOS is fully supported.
Linux support is expected but untested.
Windows support is expected but untested.


# Progress

1. Blocks can be loaded via URL
1. Decrypted blocks can be loaded via URL
1. Bundle contents can be loaded via URL
1. Validated self-contained, relative-path websites can be contained in bundles
1. Can find identifiers "like" other identifiers (first 4 nibbles or 16 bit matches)
2. Symlinks, executable, read-only, empty directories, macOS resource forks, and xattrs are preserved in bundles
3. Index file supported in bundles (file to return when looking at the root of the bundle)



# TODO

1. url stats (first, last, and count of requested, downloaded, purged with deleted and local flags)
2. Server should accept PUT for `/sha256/{identifier}`
3. Server should send node identifier and signature of response to every request
   - Send timestamp and signature of timestamp and path
   - Settings app could take url and headers and modify headers accordingly (or return headers dictionary)
   - Servers should first push their public key (unencrypted block)
4. Implement notification center (request blocks - wait with timeout, notify of blocks)
   - When requesting blocks a threading.Event is returned
   - Wait with a timeout on the event
   - If timeout, then return the status page
   - When a request comes in (if it does not already have an event) it sends the request to the connection pool
5.  Implement connection pool that takes requests and events and walks them through each of the connections (on correct order) until it finds them
   - Block id nearest to furthest from the node id
   - Thread for each connection to remote host
   - Each has a queue of requests which takes path and event
   - Each connection should do the following when connecting to a server
     - PUT its public key block (unencrypted block with json of public description, may be compressed)
     - PUT server list
     - PUT requests
     - GET requests
     - Fullfil any requests (PUT blocks or LIKE findings)
     - Send out GET requests for requested blocks and LIKEs
     - PUT some local blocks that best match this node (top 10?)
7. Implement fail-safe to prevent two server from running on the same storage (lock file with pid?)





