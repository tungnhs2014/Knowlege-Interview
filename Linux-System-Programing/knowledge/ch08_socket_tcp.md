# Chapter 8 - TCP/IP and Internet Domain Sockets

> Topics: 8.3 TCP/IP Fundamentals; 8.4 Internet Domain Sockets - layers, TCP vs UDP, byte order, `sockaddr_in`, `sockaddr_in6`, `getaddrinfo()`.
> Main sources: TLPI Ch58, Ch59; DevLinux module 06.
> Production context: network clients/servers, embedded gateways, service discovery, telemetry protocols, DNS-aware clients, and IPv4/IPv6-capable backend services.

---

## Coverage Notes

This file covers Coverage Matrix rows 8.3 and 8.4 and the Chapter 8 TCP/IP plus Internet-domain-socket Must Cover items.

- Covered here: TCP/IP layering, TCP stream semantics, UDP datagram semantics, ports, addresses, DNS/name resolution, byte order, data representation, `sockaddr_in`, `sockaddr_in6`, `sockaddr_storage`, `inet_pton()`/`inet_ntop()`, `getaddrinfo()`, `getnameinfo()`, dual-stack and IPv4/IPv6 portability, lifecycle/data flow, production bugs, debugging commands, Embedded network constraints, checklist, and interview readiness.
- Cross-file coverage: generic `socket()`/`bind()`/`listen()`/`accept()`/`connect()` lifecycle is in `ch08_socket_overview.md`; server overload and concurrency are in `ch08_socket_server.md`; nonblocking I/O, timeouts, keepalive, and advanced TCP state debugging are in `ch08_socket_advanced.md`.
- No mapped TCP/IP or Internet-domain concept is intentionally out of scope.

## Problem It Solves

Internet domain sockets let a process communicate with another process through TCP/IP. The peer may be on the same host via loopback, in the same LAN, or across the Internet.

The hard part is not calling `socket()`. The hard part is understanding what the kernel and network stack promise:

```text
application bytes/messages
        |
        v
TCP or UDP transport
        |
        v
IP packets
        |
        v
network interface / driver
```

TCP gives a reliable byte stream. UDP gives independent best-effort datagrams. IP underneath is connectionless and unreliable. Your application protocol must choose the right transport and encode data in a representation both sides understand.

## Learning Roadmap

| Level | Learn | Goal |
|-------|-------|------|
| Must know | TCP vs UDP, IP+port addressing, byte order, `sockaddr_in`, `inet_pton()`, `getaddrinfo()` | Write correct IPv4/IPv6 socket code |
| Work useful | DNS behavior, ephemeral ports, `INADDR_ANY`, stream framing, UDP size limits | Debug real network service failures |
| Recognize | IPv6 scope, `sockaddr_storage`, marshalling formats, obsolete resolver APIs | Read older code and avoid portability traps |

## Core Vocabulary

| Term | Meaning | Example / note |
|------|---------|----------------|
| Internet domain socket | Socket using TCP/IP addressing | `AF_INET` for IPv4, `AF_INET6` for IPv6 |
| IP | Network-layer protocol delivering datagrams best-effort | Packets may be lost or reordered |
| TCP | Reliable, connection-oriented byte stream over IP | `SOCK_STREAM` |
| UDP | Connectionless datagram transport over IP | `SOCK_DGRAM` |
| Port | 16-bit transport identifier for an application endpoint | HTTP often uses TCP port 80 |
| Well-known port | IANA service port, often 0-1023 | Binding usually requires `CAP_NET_BIND_SERVICE` on Linux |
| Ephemeral port | Short-lived port assigned by kernel | Client side after `connect()` |
| Network byte order | Big-endian integer representation used in protocols | Use `htons()` for ports |
| Host byte order | Native CPU integer representation | x86 is little-endian |
| `sockaddr_in` | IPv4 socket address | IP + port |
| `sockaddr_in6` | IPv6 socket address | IPv6 address + port + scope fields |
| `sockaddr_storage` | Generic storage big enough for any socket address | Useful after `accept()` |
| Presentation address | Human-readable IP string | `127.0.0.1`, `::1` |
| Binary address | Network byte-order address in struct form | Output of `inet_pton()` |
| DNS | Distributed name system mapping hostnames and IP addresses | Used by `getaddrinfo()` |
| Marshalling | Encoding data into architecture-independent format | Text lines, JSON, protobuf, XDR |
| MTU | Maximum frame payload on a link | UDP should avoid fragmentation |

## Concept Overview

### TCP/IP Layering

```text
Application protocol   HTTP, Redis protocol, custom length-prefix
Transport              TCP or UDP
Network                IPv4 or IPv6
Data link              Ethernet, Wi-Fi, cellular, loopback
```

Layering matters because each layer gives different guarantees:

| Layer | Provides | Does not provide |
|-------|----------|------------------|
| IP | Host-to-host best-effort datagrams | Reliability, ordering, application identity |
| UDP | Ports + datagram boundaries + checksum | Reliable delivery, ordering, congestion control |
| TCP | Reliable ordered byte stream, flow/congestion control | Message boundaries |
| Application | Meaning, framing, auth, retries | Kernel cannot infer protocol intent |

### TCP vs UDP

| Question | Prefer TCP | Prefer UDP |
|----------|------------|------------|
| Must every byte arrive in order? | Yes | No |
| Is the protocol a stream/conversation? | Yes | Usually no |
| Are messages small independent queries? | Sometimes | Often |
| Can the app tolerate loss or implement reliability? | Not needed | Required if reliability matters |
| Need broadcast/multicast? | No | Often yes |

If you find yourself reimplementing acknowledgements, retransmission, ordering, flow control, and congestion control over UDP, TCP is probably the right starting point.

### Data Representation

Never send raw C structs across a network as a protocol. Different machines may disagree on endian, type size, alignment, padding, and compiler ABI.

Use one of:

| Format | Good for |
|--------|----------|
| Newline-delimited text | Simple tools, telnet/netcat debugging |
| Length-prefixed binary | Efficient custom protocols |
| JSON/CBOR/protobuf | Structured cross-language messages |
| Existing protocol | HTTP, MQTT, DNS, etc. |

## System Context

| System component | Why it matters |
|------------------|----------------|
| Resolver / DNS | `getaddrinfo()` may block and may return multiple addresses |
| `/etc/hosts` | Local hostname overrides before or alongside DNS depending resolver config |
| `/etc/services` | Maps service names to port/protocol pairs |
| Kernel port table | Prevents conflicting binds and assigns ephemeral ports |
| TCP state machine | Explains `SYN_SENT`, `ESTABLISHED`, `TIME_WAIT`, `CLOSE_WAIT` |
| Network interfaces | `INADDR_ANY` binds all interfaces; specific IP binds only one address |
| Firewall/NAT | Can block packets even when code is correct |
| Byte order | Required when filling numeric socket address fields |

Production bugs often live outside the code: wrong DNS record, service bound only to loopback, port blocked by firewall, IPv6 address returned first but service listens only on IPv4, or a protocol assuming one `read()` equals one request.

## Architecture

### Internet Address Structures

```text
IPv4:
struct sockaddr_in {
    sin_family = AF_INET
    sin_port   = port in network byte order
    sin_addr   = IPv4 address in network byte order
}

IPv6:
struct sockaddr_in6 {
    sin6_family = AF_INET6
    sin6_port   = port in network byte order
    sin6_addr   = 128-bit IPv6 address
    sin6_scope_id = interface scope for link-local addresses
}
```

Use `sockaddr_storage` when a function may receive either IPv4 or IPv6.

### Special Addresses

| Address | Meaning | Common use |
|---------|---------|------------|
| `127.0.0.1` / `::1` | Loopback | Local testing, local-only service |
| `INADDR_ANY` / `in6addr_any` | Wildcard local bind | Listen on all configured interfaces |
| Port `0` | Ask kernel to choose ephemeral port | Tests, dynamic service registration |

If a service binds only to `127.0.0.1`, remote clients cannot connect even if the port is open locally.

### Conversion APIs

| API | Use | Notes |
|-----|-----|-------|
| `htons()` / `htonl()` | Host integer to network byte order | Ports and IPv4 constants |
| `ntohs()` / `ntohl()` | Network integer to host byte order | Logging ports from socket structs |
| `inet_pton()` | Presentation IP string to binary | IPv4 and IPv6 |
| `inet_ntop()` | Binary IP to presentation string | Logging without reverse DNS |
| `getaddrinfo()` | Host/service to address list | Modern, IPv4/IPv6 aware |
| `getnameinfo()` | Address to host/service string | Reverse conversion |

Obsolete APIs such as `gethostbyname()`, `inet_ntoa()`, and `getservbyname()` appear in older code. New code should prefer `getaddrinfo()`, `getnameinfo()`, `inet_pton()`, and `inet_ntop()`.

## Execution Flow

### Client with `getaddrinfo()`

```text
fill hints: AF_UNSPEC + SOCK_STREAM
    |
    v
getaddrinfo(host, service, hints) -> linked list
    |
    v
for each result:
    socket(ai_family, ai_socktype, ai_protocol)
    connect(ai_addr)
    if success: use fd
    else close and try next
    |
    v
freeaddrinfo()
```

### Server with `getaddrinfo()`

```text
fill hints: AF_UNSPEC + SOCK_STREAM + AI_PASSIVE
    |
    v
getaddrinfo(NULL, service, hints)
    |
    v
for each result:
    socket()
    setsockopt(SO_REUSEADDR)
    bind()
    if success: listen()
    else close and try next
```

### Byte Order Flow

```text
human/user port 8080
    |
    v
htons(8080)
    |
    v
addr.sin_port
    |
    v
kernel/network sees bytes in network order
```

### TCP Data Flow

```text
application write(100 KB)
    |
    v
TCP splits into segments, sequence numbers bytes
    |
    v
peer TCP reorders/retransmits as needed
    |
    v
peer read() returns however many bytes are currently available
```

### UDP Size Flow

```text
application sends one large datagram
    |
    v
IP may fragment if larger than path MTU
    |
    v
loss of one fragment loses whole datagram
```

Keep UDP messages small unless you deliberately handle fragmentation/loss. TLPI notes a conservative payload around 512 bytes is common for broad IPv4 safety.

## 8.3 / 8.4 API and Topic Sections

### TCP/IP Fundamentals

IP is best-effort. TCP adds reliability, ordering, retransmission, flow control, and congestion control. UDP adds ports and checksums but leaves reliability to the application.

Work pitfall: TCP reliability does not mean the peer processed your business message. If your protocol needs that guarantee, define an application acknowledgement.

### `sockaddr_in` and `sockaddr_in6`

Always zero-initialize address structures before filling fields. Always convert port numbers with `htons()`. For IPv4 constants like `INADDR_ANY`, use `htonl(INADDR_ANY)` in `sin_addr.s_addr`.

### `inet_pton()` and `inet_ntop()`

Use these for numeric IP strings. They do not do DNS. This is good for config values where a literal IP is required and you want predictable behavior.

### `getaddrinfo()`

Use this for hostnames, service names, and IPv4/IPv6 independence. It returns a list because a host can have multiple addresses and a service can map to different socket types.

Common hints:

| Hint | Use |
|------|-----|
| `AF_UNSPEC` | Accept IPv4 or IPv6 |
| `SOCK_STREAM` | TCP results |
| `SOCK_DGRAM` | UDP results |
| `AI_PASSIVE` | Server bind addresses |
| `AI_NUMERICHOST` | Avoid DNS; host must be numeric |
| `AI_NUMERICSERV` | Service must be numeric port |

### DNS and Services

DNS lookup may block and may fail temporarily. `/etc/services` maps service names such as `http` to port/protocol pairs, but it does not reserve the port for your application.

## Work Checklist

| Pattern | Why it matters |
|---------|----------------|
| Prefer `getaddrinfo(AF_UNSPEC)` | Handles IPv4/IPv6 without duplicate code |
| Loop through all returned addresses | First DNS result may not connect |
| Log numeric peer addresses for hot paths | Avoid slow reverse DNS in request path |
| Use `sockaddr_storage` for peer addresses | Keeps accept/recvfrom code version-independent |
| Define stream framing | TCP does not preserve application messages |
| Use text/newline protocols for simple tools | Easy to test with `nc`/`telnet` |
| Keep UDP datagrams bounded | Reduces fragmentation loss |
| Bind to explicit interface when needed | `0.0.0.0` exposes service on all interfaces |
| Treat DNS errors separately from `errno` | Use `gai_strerror()` for `getaddrinfo()` errors |

## Advanced / Recognize First

| Topic | Know this much |
|-------|----------------|
| IPv6 link-local scope | Needs `sin6_scope_id` to identify interface |
| IPv4-mapped IPv6 | Compatibility behavior controlled by system/socket options |
| `AI_ADDRCONFIG` | Filters results based on local configured address families; glibc defaults differ when hints is NULL |
| Path MTU discovery | TCP handles this better than UDP; UDP apps must design around size |
| Obsolete resolver APIs | `gethostbyname()` and `inet_ntoa()` are common in legacy code but poor for new code |
| Raw sockets | Bypass normal TCP/UDP APIs; specialized and privilege-sensitive |

## Example

### Example 1 - Protocol-Independent TCP Client

```c
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>

static int write_all(int fd, const char *buf, size_t len)
{
    while (len > 0) {
        ssize_t n = write(fd, buf, len);
        if (n == -1) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        buf += n;
        len -= (size_t)n;
    }
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s host port\n", argv[0]);
        return EXIT_FAILURE;
    }

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV;

    struct addrinfo *result;
    int s = getaddrinfo(argv[1], argv[2], &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        return EXIT_FAILURE;
    }

    int fd = -1;
    for (struct addrinfo *rp = result; rp != NULL; rp = rp->ai_next) {
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd == -1) {
            continue;
        }
        if (connect(fd, rp->ai_addr, rp->ai_addrlen) == 0) {
            break;
        }
        close(fd);
        fd = -1;
    }

    freeaddrinfo(result);

    if (fd == -1) {
        fprintf(stderr, "could not connect\n");
        return EXIT_FAILURE;
    }

    const char msg[] = "ping\n";
    if (write_all(fd, msg, strlen(msg)) == -1) {
        perror("write");
        close(fd);
        return EXIT_FAILURE;
    }

    close(fd);
    return EXIT_SUCCESS;
}
```

What it teaches:

- New code should try all `getaddrinfo()` results.
- `getaddrinfo()` errors are reported with `gai_strerror()`, not `perror()`.
- The client does not need to bind; the kernel assigns an ephemeral local port.

### Example 2 - Protocol-Independent TCP Listener

```c
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>

#define BACKLOG 32
#define BUF_SIZE 256
#define HOST_LEN 1025
#define SERV_LEN 32

static void die(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s port\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        die("signal");
    }

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

    struct addrinfo *result;
    int s = getaddrinfo(NULL, argv[1], &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        return EXIT_FAILURE;
    }

    int lfd = -1;
    int yes = 1;
    for (struct addrinfo *rp = result; rp != NULL; rp = rp->ai_next) {
        lfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (lfd == -1) {
            continue;
        }
        if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
            close(lfd);
            lfd = -1;
            continue;
        }
        if (bind(lfd, rp->ai_addr, rp->ai_addrlen) == 0) {
            break;
        }
        close(lfd);
        lfd = -1;
    }

    freeaddrinfo(result);

    if (lfd == -1) {
        fprintf(stderr, "could not bind\n");
        return EXIT_FAILURE;
    }
    if (listen(lfd, BACKLOG) == -1) {
        die("listen");
    }

    for (;;) {
        struct sockaddr_storage peer;
        socklen_t peer_len = sizeof(peer);
        int cfd = accept(lfd, (struct sockaddr *)&peer, &peer_len);
        if (cfd == -1) {
            if (errno == EINTR) {
                continue;
            }
            die("accept");
        }

        char host[HOST_LEN];
        char service[SERV_LEN];
        s = getnameinfo((struct sockaddr *)&peer, peer_len,
                        host, sizeof(host), service, sizeof(service),
                        NI_NUMERICHOST | NI_NUMERICSERV);
        if (s == 0) {
            printf("connection from %s:%s\n", host, service);
        }

        char buf[BUF_SIZE];
        ssize_t n = read(cfd, buf, sizeof(buf));
        if (n > 0) {
            (void)write(cfd, buf, (size_t)n);
        }
        close(cfd);
    }
}
```

What it teaches:

- Server bind code should be IPv4/IPv6 agnostic when possible.
- `AI_PASSIVE` returns wildcard bind addresses.
- `sockaddr_storage` works for accepted peer addresses.

### Example 3 - Byte Order and Address Conversion

```c
#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int main(void)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);

    if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) != 1) {
        fprintf(stderr, "invalid address\n");
        return 1;
    }

    char text[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &addr.sin_addr, text, sizeof(text)) == NULL) {
        perror("inet_ntop");
        return 1;
    }

    printf("%s:%u\n", text, (unsigned)ntohs(addr.sin_port));

    uint32_t host_value = 0x01020304u;
    uint32_t network_value = htonl(host_value);
    printf("host=0x%08x network=0x%08x\n", host_value, network_value);

    return 0;
}
```

What it teaches:

- Ports in socket structures must be in network byte order.
- Use `inet_pton()`/`inet_ntop()` for numeric IP conversion.
- Printing a network-order integer directly can confuse debugging.

## Debugging

### Common Bugs

| Bug | Symptom | Fix / check |
|-----|---------|-------------|
| Missing `htons()` | Server listens on unexpected port | Log `ntohs(addr.sin_port)` |
| Binding to loopback only | Remote clients cannot connect | Check `ss -ltnp` local address |
| Assuming one `read()` is one TCP message | Protocol desync | Add framing |
| DNS blocks request path | Latency spikes | Resolve outside hot path or use numeric logging |
| Only trying first `getaddrinfo()` result | Works on one network, fails on another | Iterate all addresses |
| UDP datagram too large | Random loss under network path | Keep bounded or implement retry/framing |
| Reverse DNS in logs | Slow accept/request logging | Use `NI_NUMERICHOST` |
| IPv6 ignored | Client fails when DNS returns IPv6 first | Use `AF_UNSPEC` and test both |

### Commands

```bash
# Listening sockets and bound addresses
ss -ltnup

# TCP states and peer addresses
ss -tan

# Resolve like the system resolver
getent ahosts example.com

# DNS-specific lookup
dig example.com A
dig example.com AAAA

# Check service name mapping
grep -w '^http' /etc/services

# Trace resolver and connect flow
strace -f -e trace=network,connect,recvfrom,sendto ./client example.com 80

# Inspect ephemeral port range
cat /proc/sys/net/ipv4/ip_local_port_range

# Capture traffic for a port
sudo tcpdump -n 'port 8080'
```

## Real-world Usage

| Scenario | Practical design |
|----------|------------------|
| Public backend API | TCP, IPv4/IPv6 aware bind, robust framing, TLS above socket |
| Local-only admin endpoint | Bind to loopback or use UNIX domain socket |
| DNS client/server | UDP for common small query, TCP fallback for large transfers |
| Telemetry stream | TCP if delivery/order matter; UDP if loss is acceptable and bounded |
| Embedded device config | TCP command channel with explicit request/response framing |
| Health check service | Small TCP or UDP endpoint, numeric logs, short timeouts |

## Interview Readiness

1. What guarantees does IP provide, and what does it not provide?
2. Compare TCP and UDP at the transport layer.
3. Why does TCP provide a byte stream rather than message boundaries?
4. What is a port number used for?
5. What is an ephemeral port, and when is one assigned?
6. Why must socket address fields use network byte order?
7. When do you use `htons()` versus `htonl()`?
8. What is `INADDR_ANY`, and how is it different from `127.0.0.1`?
9. Why should new code prefer `getaddrinfo()` over `gethostbyname()`?
10. Why does `getaddrinfo()` return a linked list?
11. What is the difference between `inet_pton()` and DNS resolution?
12. Why should servers avoid reverse DNS in a hot path?
13. What can go wrong with large UDP datagrams?
14. Why should you not send raw C structs over the network?
15. How would you debug a server that works on localhost but not remotely?
16. How would you make a TCP client work with both IPv4 and IPv6?

## Key Takeaways

1. Internet sockets use IP address plus port as the endpoint identity.
2. IP is best-effort; TCP or the application adds stronger semantics.
3. TCP is reliable and ordered but has no application message boundaries.
4. UDP preserves datagrams but does not guarantee delivery or order.
5. Ports and IPv4 addresses in socket structs must be in network byte order.
6. Use `getaddrinfo()` for modern IPv4/IPv6 code.
7. Try every `getaddrinfo()` result, not just the first.
8. `sockaddr_storage` is the practical generic peer address buffer.
9. DNS and reverse DNS can block and should be handled deliberately.
10. `INADDR_ANY` exposes a server on all matching local interfaces.
11. UDP apps must consider MTU, fragmentation, loss, and retry policy.
12. Network protocols need explicit data representation and framing.
