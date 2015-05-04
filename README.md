# cripple

[![Build Status](https://travis-ci.org/hmgle/cripple.png?branch=master)](https://travis-ci.org/hmgle/cripple)

[中文](README_zh.md)

## Introduction

Cripple is a server like stund based on STUN [RFC 3489](http://tools.ietf.org/html/rfc3489), but not fully compatible with STUN. It only need single public IP address to run the server. Now cripple only reply the Binding Requests. A client can get the NAT information via cripple service.

## Feature Summary

The biggest difference with stund is that cripple only need single network interface with public IP address to run server.

## Building

The following libraries need to install before build:

- libev-dev
- libnet1-dev
- libssl-dev

Debian/Ubuntu Installation:

```
sudo apt-get install libev-dev libnet1-dev libssl-dev
```

Then run the following:

```
git clone https://github.com/hmgle/cripple.git
cd cripple
make
```

Summary for executable files:

- `server`: Like stund, need root privileges to run
- `client.py`: Like pystun
- `forge_ip_server`: It provide the service that detect Fake IP Address IP packet whether can be sent
- `forge_ip_client`: It check the host if can send the Fake IP Address IP packet, need root privileges to run

## FAQ

### Stund server is already exists, why do you make this cripple?

I want to run my stund server on my VPS to help P2P communication, but this VPS only has one public IP address. As stund need two public IP address, cripple just need one, mean it can run on my VPS.

### How can it provide STUN service with single network interface? 

The server will modify the source IP in IP protocol stack, fake another IP address data packet to reply the "change IP" request from client. The client never send UDP data to the fake IP address, so it is **incompatible** the STUN protocol. You should use `client.py`, not `pystun`.

### How can I get the NAT type my host behind?

Just run the `client.py`. I have run the cripple `server` on my VPS (138.128.215.119).

Example:

```console
$ ./client.py -H 138.128.215.119 # ./client.py -H 138.128.215.119 -i 你的IP更好
NAT Type: Symmetric NAT
External IP: 58.251.211.73
External Port: 7296
```

See? Your host is behind the Symmetric NAT.

### I want to run cripple server on my host.

First make sure you host has a public IP address, e.g., VPS. Then make sure the ISP or firewall will not intercept Fake IP UPD data packets. It can check by this(I have run a `forge_ip_server` on my VPS 138.128.215.119):

```console
$ sudo ./forge_ip_client -i 123.45.67.89 -s 138.128.215.119
forged source port: 1234 ip: 123.45.67.89 # show this mean your host can send fake IP packet
```

But if this reply:

```console
$ sudo ./forge_ip_client -i 123.45.67.89 -s 138.128.215.119
Block!
```
mean ISP or firewall intercept abnormal IP Packets.

After pass this check, then run the cripple with root:

```console
$ sudo ./server -b
```

## License

Cripple is released under MIT License. See `LICENSE` for more information.
