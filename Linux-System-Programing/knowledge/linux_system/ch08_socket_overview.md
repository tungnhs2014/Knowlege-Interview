# Sockets Introduction — socket/bind/listen/accept/connect

## Problem It Solves

All IPC mechanisms covered up to this point (pipes, FIFOs, System V / POSIX IPC) operate
only **within a single host** — two processes must share the same kernel to use them.

Real-world systems require communication between **processes on different machines** over a
network: web servers, databases, distributed systems. Even for same-host communication, a
unified API that works identically regardless of whether the peer is local or remote is
valuable for architecture flexibility.

**Sockets** solve both problems:
- Provide IPC between processes on **different hosts** over a network
- Provide **the same API** for local communication (`AF_UNIX`) and network communication
  (`AF_INET`, `AF_INET6`)
- Integrate naturally into the Unix file model — socket endpoints are file descriptors,
  so `read()`, `write()`, `close()`, `select()`, `poll()` all work on them

First introduced in **4.2BSD (1983)**, sockets became the universal IPC API for networked
applications and are now specified in POSIX.1 (SUSv3).

---

## Concept Overview

A socket is a **communication endpoint** represented in userspace as a file descriptor. Both
sides of a communication channel must create their own socket. Three parameters identify a
socket's character:

| Parameter | Controls | Common values |
|-----------|----------|---------------|
| **domain** | Address format + communication scope | `AF_UNIX`, `AF_INET`, `AF_INET6` |
| **type** | Data transfer semantics | `SOCK_STREAM`, `SOCK_DGRAM` |
| **protocol** | Specific protocol (usually 0 = auto) | `0`, `IPPROTO_TCP`, `IPPROTO_UDP` |

```c
int fd = socket(domain, type, protocol);
```

### Communication Domains

| Domain | Address format | Communication scope | Address structure |
|--------|---------------|---------------------|-------------------|
| `AF_UNIX` | Filesystem pathname | Same host only, within kernel | `sockaddr_un` |
| `AF_INET` | 32-bit IPv4 + 16-bit port | Across IPv4 network | `sockaddr_in` |
| `AF_INET6` | 128-bit IPv6 + 16-bit port | Across IPv6 network | `sockaddr_in6` |

> Note: `AF_` stands for "Address Family", `PF_` ("Protocol Family") is a historical
> synonym. SUSv3 only specifies `AF_` constants — always use those.

### Socket Types

| Property | `SOCK_STREAM` | `SOCK_DGRAM` |
|----------|--------------|-------------|
| Reliability | Guaranteed delivery | Unreliable (may drop, duplicate, reorder) |
| Message boundaries | None (byte-stream) | Preserved (each send = one datagram) |
| Connection | Required (connection-oriented) | Not required (connectionless) |
| Protocol (AF_INET) | TCP | UDP |
| Analogy | Telephone call | Post office |

### Generic Address Structure

Each domain has its own address structure (`sockaddr_un`, `sockaddr_in`, `sockaddr_in6`).
System calls like `bind()` and `connect()` must accept any of them. The solution is a
**generic wrapper** `struct sockaddr` used only for casting:

```c
struct sockaddr {
    sa_family_t sa_family;  /* AF_UNIX / AF_INET / AF_INET6 */
    char        sa_data[14];
};
```

In practice: define a domain-specific struct, then cast to `(struct sockaddr *)` when
passing to system calls.

---

## System Context

Sockets sit at the junction between the **VFS layer** and the **network stack**:

```
userspace process
      │  fd = socket(...)
      ▼
 file descriptor table
      │
      ▼
  struct file  ──►  struct socket  (kernel socket object)
                          │
              ┌───────────┴───────────┐
              │                       │
        AF_UNIX path             AF_INET path
        (stays in kernel,        (goes through
         no real network)         TCP/IP stack)
                                       │
                                  NIC driver
                                       │
                                  network wire
```

Because sockets are file descriptors:
- They appear in `/proc/<pid>/fd/`
- They obey `close-on-exec` (`SOCK_CLOEXEC` flag)
- They work with `select()`, `poll()`, `epoll()`, `fcntl()`
- They are reference-counted — connection closes only when all referring FDs are closed

**Port privilege boundary**: on Linux, binding to ports `< 1024` ("well-known ports") requires
`CAP_NET_BIND_SERVICE`. Ports `1024–65535` are available to any process. Ephemeral ports
(kernel-assigned for clients) are drawn from `/proc/sys/net/ipv4/ip_local_port_range`
(default `32768–60999`).

---

## Architecture

### Key System Calls

```
socket()  ──── create an endpoint, returns fd
  │
bind()    ──── attach a local address to the socket
  │
listen()  ──── (server only) mark socket as passive, create accept queue
  │
accept()  ──── (server only) dequeue a completed connection, return new fd
  │
connect() ──── (client) initiate connection to a remote address
  │
read()/write()
send()/recv()   ──── data transfer on connected sockets
  │
sendto()/recvfrom()  ──── data transfer + address for datagram sockets
  │
close()   ──── release the socket fd
```

### Stream Socket Architecture (Server side)

```
                 listenfd (passive socket)
                      │
              ┌───────┴────────────────┐
              │   accept queue         │  ← backlog limit
              │  [conn1][conn2][conn3] │
              └───────────────────────┘
                   │         │
              connfd1     connfd2     ← each accept() creates a NEW socket
                   │         │
              client1     client2     ← independent bidirectional channels
```

The listening socket (`listenfd`) is **permanent** — it never connects to a peer.
Each `accept()` creates a distinct connected socket (`connfd`) with its own 4-tuple:
`(server_ip, server_port, client_ip, client_port)`.

### Datagram Socket Architecture

```
server_sock ──── bind() to well-known address
                      │
              recvfrom() ──── receives datagram + sender address
                      │
              sendto()  ──── sends reply to sender address

(no accept queue, no connected sockets, no connection state)
```

### TCP Connection Queue (Kernel Internals)

Linux TCP implementation uses **two queues** behind `listen()`:

```
SYN queue (incomplete connections)
  ─ populated when SYN arrives from client
  ─ kernel sends SYN-ACK
  ─ limited by /proc/sys/net/ipv4/tcp_max_syn_backlog

accept queue (completed connections)
  ─ populated when 3-way handshake completes (ACK received)
  ─ drained by application's accept() calls
  ─ limited by the backlog argument to listen()
  ─ capped by /proc/sys/net/core/somaxconn (default 128)
```

When the accept queue is full, the kernel **drops new SYN packets** (or sends RST depending
on `/proc/sys/net/ipv4/tcp_abort_on_overflow`). The client sees a timeout and retries.

---

## Execution Flow

### Stream Socket (TCP) — Full Lifecycle

```
SERVER                                    CLIENT
──────────────────────────────────────────────────────────
socket(AF_INET, SOCK_STREAM, 0)
  │  → kernel allocates struct socket
bind(listenfd, &serveraddr, addrlen)
  │  → kernel registers addr:port → listenfd
listen(listenfd, backlog)
  │  → socket state: CLOSED → LISTEN
  │  → accept queue created

accept(listenfd, &clientaddr, &addrlen)   socket(AF_INET, SOCK_STREAM, 0)
  │  BLOCKS (no connections)
  │                                       connect(sockfd, &serveraddr, addrlen)
  │                                         │  → kernel sends SYN
  │  ◄─────────────── SYN ─────────────────┤
  │  SYN-ACK ──────────────────────────────►
  │                                         │  → 3-way handshake complete
  │  ◄─────────────── ACK ─────────────────┤
  │  → connection moved to accept queue     │  → connect() returns 0
  │  → accept() returns connfd              │
  │
read(connfd, buf, size)                   write(sockfd, data, len)
  │  ◄─────────────── data ────────────────┤
  │
write(connfd, reply, len)                 read(sockfd, buf, size)
  │  ─────────────── reply ───────────────►│

close(connfd)                             [continues or close()]
  │  → FIN sent
  │  ─────────────── FIN ────────────────►
  │                                       read() returns 0 (EOF)
  │                                       close(sockfd)
  │  ◄─────────────── FIN ─────────────────
```

### Datagram Socket (UDP) — Lifecycle

```
SERVER                                    CLIENT
──────────────────────────────────────────────────────────
socket(AF_INET, SOCK_DGRAM, 0)            socket(AF_INET, SOCK_DGRAM, 0)
bind(&serveraddr)                         (no bind needed — kernel assigns
                                           ephemeral port on first sendto)

recvfrom(sockfd, buf, &clientaddr)        sendto(sockfd, data, &serveraddr)
  │  BLOCKS                                 │
  │  ◄────────── datagram ─────────────────┤
  │  → returns datagram + clientaddr

sendto(sockfd, reply, &clientaddr)        recvfrom(sockfd, buf, NULL)
  │  ──────── datagram ───────────────────►│
```

---

## Example (Code)

### Example 1 — TCP Echo Server + Client

```c
/* tcp_server.c — Iterative TCP Echo Server
 * Compile: gcc tcp_server.c -o tcp_server
 * Run:     ./tcp_server 8080
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUF_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 2) { fprintf(stderr, "Usage: %s <port>\n", argv[0]); exit(1); }

    /* 1. Create socket: AF_INET=IPv4, SOCK_STREAM=TCP, 0=auto-protocol */
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1) { perror("socket"); exit(1); }

    /* SO_REUSEADDR: allow immediate rebind after server restart.
     * Without this, the port stays in TIME_WAIT for ~60s after close. */
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* 2. Bind: attach INADDR_ANY:port to socket.
     * htonl/htons convert host byte order → network byte order (big-endian). */
    struct sockaddr_in serveraddr = {
        .sin_family      = AF_INET,
        .sin_addr.s_addr = htonl(INADDR_ANY),
        .sin_port        = htons(atoi(argv[1])),
    };
    if (bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) == -1) {
        perror("bind"); exit(1);
    }

    /* 3. Listen: transition to passive state, accept queue depth = 5 */
    if (listen(listenfd, 5) == -1) { perror("listen"); exit(1); }
    printf("Server listening on port %s...\n", argv[1]);

    while (1) {
        /* 4. Accept: block until a completed connection is in the accept queue.
         * Returns a NEW fd (connfd) for this specific client connection.
         * listenfd stays open and continues accepting further connections. */
        struct sockaddr_in clientaddr;
        socklen_t addrlen = sizeof(clientaddr);
        int connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &addrlen);
        if (connfd == -1) { perror("accept"); continue; }

        printf("Client connected: %s:%d\n",
               inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

        /* 5. Echo loop: read from client, write back */
        char buf[BUF_SIZE];
        ssize_t n;
        while ((n = read(connfd, buf, sizeof(buf))) > 0)
            write(connfd, buf, n);

        /* 6. Close connected socket; listenfd remains open */
        close(connfd);
        printf("Client disconnected\n");
    }

    close(listenfd);
    return 0;
}
```

```c
/* tcp_client.c — TCP Client
 * Compile: gcc tcp_client.c -o tcp_client
 * Run:     ./tcp_client 127.0.0.1 8080
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]) {
    if (argc != 3) { fprintf(stderr, "Usage: %s <ip> <port>\n", argv[0]); exit(1); }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) { perror("socket"); exit(1); }

    struct sockaddr_in serveraddr = {
        .sin_family = AF_INET,
        .sin_port   = htons(atoi(argv[2])),
    };
    /* inet_pton: dotted-decimal string → binary IPv4 in network byte order */
    if (inet_pton(AF_INET, argv[1], &serveraddr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid address\n"); exit(1);
    }

    /* connect() performs TCP 3-way handshake; blocks until complete.
     * On failure: must close() this socket and create a new one to retry. */
    if (connect(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) == -1) {
        perror("connect"); exit(1);
    }
    printf("Connected to server\n");

    const char *msg = "Hello, Socket!";
    write(sockfd, msg, strlen(msg));

    char buf[256] = {0};
    ssize_t n = read(sockfd, buf, sizeof(buf) - 1);
    if (n > 0) printf("Echo: %s\n", buf);

    close(sockfd);
    return 0;
}
```

### Example 2 — UDP Datagram Echo Server + Client

```c
/* udp_server.c — UDP Echo Server
 * Compile: gcc udp_server.c -o udp_server
 * Run:     ./udp_server 9090
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUF_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 2) { fprintf(stderr, "Usage: %s <port>\n", argv[0]); exit(1); }

    /* SOCK_DGRAM = UDP — no listen()/accept() needed */
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) { perror("socket"); exit(1); }

    struct sockaddr_in serveraddr = {
        .sin_family      = AF_INET,
        .sin_addr.s_addr = htonl(INADDR_ANY),
        .sin_port        = htons(atoi(argv[1])),
    };
    if (bind(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) == -1) {
        perror("bind"); exit(1);
    }
    printf("UDP server listening on port %s...\n", argv[1]);

    char buf[BUF_SIZE];
    while (1) {
        struct sockaddr_in clientaddr;
        socklen_t addrlen = sizeof(clientaddr);

        /* recvfrom: receive one datagram + capture sender's address */
        ssize_t n = recvfrom(sockfd, buf, sizeof(buf), 0,
                             (struct sockaddr *)&clientaddr, &addrlen);
        if (n == -1) { perror("recvfrom"); continue; }

        printf("Datagram from %s:%d (%zd bytes)\n",
               inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port), n);

        /* sendto: send reply directly to sender's address */
        sendto(sockfd, buf, n, 0, (struct sockaddr *)&clientaddr, addrlen);
    }

    close(sockfd);
    return 0;
}
```

```c
/* udp_client.c — UDP Client using connect() to simplify I/O
 * Compile: gcc udp_client.c -o udp_client
 * Run:     ./udp_client 127.0.0.1 9090
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]) {
    if (argc != 3) { fprintf(stderr, "Usage: %s <ip> <port>\n", argv[0]); exit(1); }

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) { perror("socket"); exit(1); }

    struct sockaddr_in serveraddr = {
        .sin_family = AF_INET,
        .sin_port   = htons(atoi(argv[2])),
    };
    inet_pton(AF_INET, argv[1], &serveraddr.sin_addr);

    /* connect() on UDP socket: kernel records peer address.
     * After this, write()/read() can be used instead of sendto()/recvfrom().
     * Side effect: only datagrams FROM this peer will be received. */
    if (connect(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) == -1) {
        perror("connect"); exit(1);
    }

    const char *msg = "UDP Hello!";
    write(sockfd, msg, strlen(msg));

    char buf[256] = {0};
    read(sockfd, buf, sizeof(buf) - 1);
    printf("Echo: %s\n", buf);

    close(sockfd);
    return 0;
}
```

### Example 3 — Byte Order & Address Conversion

```c
/* addr_demo.c — Demonstrate byte order and address conversion utilities
 * Compile: gcc addr_demo.c -o addr_demo
 * Run:     ./addr_demo
 */
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>

static void print_bytes(const char *label, const void *ptr, size_t n) {
    const unsigned char *p = ptr;
    printf("%-26s: ", label);
    for (size_t i = 0; i < n; i++) printf("%02X ", p[i]);
    printf("\n");
}

int main(void) {
    /* Port 8080 = 0x1F90
     * On little-endian (x86): stored as 90 1F in memory.
     * Network byte order (big-endian): stored as 1F 90. */
    uint16_t host_port = 8080;
    uint16_t net_port  = htons(host_port);
    print_bytes("host port 8080", &host_port, 2);
    print_bytes("network port   ", &net_port, 2);

    printf("\n");

    /* inet_pton: string → binary (network byte order) — thread-safe */
    struct in_addr addr;
    inet_pton(AF_INET, "192.168.1.100", &addr);
    print_bytes("192.168.1.100 binary", &addr, 4);

    /* inet_ntop: binary → string — thread-safe, prefer over inet_ntoa */
    char str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr, str, sizeof(str));
    printf("inet_ntop result          : %s\n", str);

    printf("\n");

    /* Correct way to fill sockaddr_in */
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port   = htons(8080);
    inet_pton(AF_INET, "192.168.1.100", &sa.sin_addr);

    printf("sizeof(sockaddr_in)       : %zu bytes\n", sizeof(sa));
    printf("sin_family (AF_INET=%d)   : %d\n", AF_INET, sa.sin_family);
    printf("sin_port (network order)  : 0x%04X\n", sa.sin_port);

    return 0;
}
```

---

## Debugging

### Common Errors

| Error | Cause | Fix |
|-------|-------|-----|
| `EADDRINUSE` on `bind()` | Port still in `TIME_WAIT` after server restart | Set `SO_REUSEADDR` before `bind()` |
| `ECONNREFUSED` on `connect()` | No server listening on that port, or firewall RST | Check server is running; verify port |
| `EACCES` on `bind()` | Port < 1024 without root privilege | Use port ≥ 1024 or grant `CAP_NET_BIND_SERVICE` |
| `EMFILE` on `accept()` | Process FD limit reached (leaked connfd) | Always `close(connfd)` after handling each client |
| `EPIPE` on `write()` | Peer closed connection while we were writing | Ignore `SIGPIPE` (`signal(SIGPIPE, SIG_IGN)`), check `EPIPE` errno |
| `ETIMEDOUT` on `connect()` | No route to host, or host down | Retry with new socket (cannot reuse same socket after failed connect) |

### Inspection Commands

```bash
# List all listening sockets with pid and fd
ss -tlnp

# Show full connection state (ESTABLISHED, TIME_WAIT, etc.)
ss -tanp

# Check accept queue overflow (listen backlog drops)
ss -tlnp | grep <port>
netstat -s | grep -i "listen"

# View socket fd of a specific process
ls -la /proc/<pid>/fd/ | grep socket
cat /proc/net/tcp          # raw TCP socket table (hex addresses)

# Check somaxconn (accept queue upper bound)
cat /proc/sys/net/core/somaxconn

# Check ephemeral port range
cat /proc/sys/net/ipv4/ip_local_port_range
```

### strace Snippet

```bash
# Trace socket system calls of a server binary
strace -e trace=socket,bind,listen,accept,connect,read,write ./tcp_server 8080
```

---

## Real-world Usage

| Pattern | Socket configuration | Example |
|---------|---------------------|---------|
| HTTP/HTTPS server | `AF_INET SOCK_STREAM` port 80/443 | nginx, Apache |
| Local service (systemd socket activation) | `AF_UNIX SOCK_STREAM` | DBus, PostgreSQL, Docker daemon |
| DNS resolver | `AF_INET SOCK_DGRAM` port 53 | bind9, unbound |
| Video streaming | `AF_INET SOCK_DGRAM` (RTP/UDP) | WebRTC, VLC |
| SSH | `AF_INET SOCK_STREAM` port 22 | OpenSSH |
| Metrics collection (same host) | `AF_UNIX SOCK_DGRAM` | statsd, syslog |

**Socket activation (systemd)**: systemd creates the listening socket *before* the service
starts. When the first connection arrives, systemd starts the service and passes the
pre-bound fd via `SD_LISTEN_FDS_START`. The service never calls `socket()/bind()/listen()` —
it inherits them. This enables zero-downtime restarts.

---

## Key Takeaways

| Concept | Core point |
|---------|-----------|
| `socket()` | Creates an endpoint; kernel allocates `struct socket` — returns an fd |
| `bind()` | Attaches a local address; port < 1024 requires `CAP_NET_BIND_SERVICE` |
| `listen()` | Transitions to passive; two-queue model (SYN queue + accept queue) |
| `accept()` | Creates a **new** connected socket per client; listening socket unchanged |
| `connect()` | TCP: triggers 3-way handshake; UDP: records peer address only |
| `SOCK_STREAM` | Reliable, ordered byte-stream; no message boundaries; must connect first |
| `SOCK_DGRAM` | Unreliable; message boundaries preserved; no connection required |
| Byte order | Always use `htons`/`htonl` when writing to `sockaddr`; `ntohs`/`ntohl` when reading |
| `struct sockaddr` | Generic cast target — define domain-specific struct, cast to `(struct sockaddr *)` |
| Failed `connect()` | Close the socket and create a new one — cannot retry on the same socket |
| `SO_REUSEADDR` | Set before `bind()` on all servers to avoid `EADDRINUSE` on restart |
