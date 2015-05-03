# cripple

[![Build Status](https://travis-ci.org/hmgle/cripple.png?branch=master)](https://travis-ci.org/hmgle/cripple)

cripple 是一个在 STUN (UDP 简单穿透 NAT) [RFC 3489](http://tools.ietf.org/html/rfc3489) 协议基础上修改的实现. 目前服务器仅对客户端的 Binding 请求回应.

最大的特点是它仅需要一个网络接口就可以提供检测 NAT 类型服务了.

## FAQ

### 已经有 stund 了, 为什么还造这样一个轮子?

我想在我的 VPS 上运行自己的 stund, 以便获取我的网络信息后进行 UDP 打洞进行 P2P 通信.可是 stund 需要两个有公网地址的网卡才能运行. cripple 可以让只有一个公网地址的主机也能提供客户端获取 NAT 映射后的地址信息的服务.

### 它是如何实现单网卡提供 STUN 服务的?

服务器通过修改 IP 协议栈中的源 IP 地址信息, 伪造出另一个 IP 数据包, 用来回应客户端的 "change IP" 请求. 而客户端需要注意的是获取这个伪造地址的数据包后绝不要发任何请求给这个来源地址. 因此它**不兼容** STUN 协议, 目前客户端仅能用目录内的 `client.py` 来获取 NAT 信息, 而不能用 `pystun`.

### 我如何用它来得到自己的电脑位于何种类型的 NAT 之后?

你需要知道一台已经运行了 cripple 服务的主机地址. 我在搬瓦工运行了一个了呢, 不过网络可能不太稳定, 地址是: 138.128.215.119, 然后:

```console
$ ./client.py -H 138.128.215.119 # ./client.py -H 138.128.215.119 -i 你的IP更好
NAT Type: Symmetric NAT
External IP: 58.251.211.73
External Port: 7296
```
呵呵, 很遗憾检测出这台主机位于对称型 NAT 之后:(

### 我想运行自己的 cripple 服务器呢?

首先你需要一台有公网地址的主机, 比如你租的 VPS. 确保网络服务商不会拦截伪造 IP 的数据包. 这可以通过 `forge_ip_client` 来检测. 我在搬瓦工已经跑了一个 `forge_ip_server`.

```console
$ sudo ./forge_ip_client -i 123.45.67.89 -s 138.128.215.119
forged source port: 1234 ip: 123.45.67.89 # 收到这样的返回, 表示可以发出伪造数据包
```

如果是这样的返回:

```console
$ sudo ./forge_ip_client -i 123.45.67.89 -s 138.128.215.119
Block!
```
说明服务器没有返回消息, 要么是你的网络服务提供商或防火墙不允许异常 IP 网络包发出, 要么是中间过程丢包了.

如果通过检测, 那么在这台主机上以超级用户身份继续运行 cripple 服务就可以了:

```console
$ sudo ./server -b
```

