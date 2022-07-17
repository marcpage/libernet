
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

Note: We could use ZeroConf


# Platform Support

macOS is fully supported.
Linux support is expected but untested.
Windows support is expected but untested.


# Progress

1. Blocks can be loaded via URL
1. Decrypted blocks can be loaded via URL
1. Bundle contents can be loaded via URL
1. Validated self-contained, relative-path websites can be contained in bundles
2. Can find identifiers "like" other identifiers (first 4 nibbles or 16 bit matches)


# TODO

1. url stats (first, last, and count of requested, downloaded, purged with deleted and local flags)
1. Create a node identifier if ones does not exist in storage
1. Server should send node identifier and signature of response to every request
   - Send timestamp and signature of timestamp and path
1. Implement notification center (request blocks - wait with timeout, notify of blocks)
   - When requesting blocks a threading.Event is returned
   - Wait with a timeout on the event
   - If timeout, then return the status page
   - When a request comes in (if it does not already have an event) it sends the request to the connection pool
1. Implement connection pool that takes requests and events and walks them through each of the connections (on correct order) until it finds them
   - Block id nearest to furthest from the node id
   - Thread for each connection to remote host
   - Each has a queue of requests which takes path and event





