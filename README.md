# vpngate_io

A library for working with the vpngate .dat file:

- decrypting it
- deserializing custom binary serialized Pack fields into usable data
- unpacking the Pack-in-Pack (possibly compressed) data

Also comes with some utilities:
- A .dat to json conversion utility

# Why?

I will forever drill this into everyone who was involved with this:

**security through obscurity is not security**. No buts.

There is little to no security besides obscurity in this whatsoever:

- IP lists are "hidden" by partial geo fencing
- (reportedly) fake list entries are pressent when scraping other data sources
- The dat file uses RC4 (Yes, really) with a key derived in the dumbest fashion I can possibly think of

This has slowly seemed to become a pattern.