
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

Note: We could use ZeroConf DNS-SD to advertise ourselves on the local network.
https://github.com/mjansson/mdns

# Platform Support

macOS is fully supported.
Linux support is expected but untested.
Windows support is expected but untested.

# Progress

1. Blocks can be loaded via URL
2. Decrypted blocks can be loaded via URL
3. Bundle contents can be loaded via URL
4. Validated self-contained, relative-path websites can be contained in bundles


# TODO

1. Implement /sha256/like/\{identifier\}



