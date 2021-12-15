# Networking

Xelix uses the [PicoTCP network stack](https://github.com/tass-belgium/picotcp), which fully supports IP/ICMP/UDP/TCP. However, PicoTCP has its own interface and is not task-aware, so Xelix has a shim to translate between a BSD socket interface and PicoTCP.
