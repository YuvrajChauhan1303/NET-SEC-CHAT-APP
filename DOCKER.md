# Docker Architecture of the Secure Chat Application

## 1. Overview

The project uses a **containerized client-server architecture with a
separate Certificate Authority (CA)**.

The system has three logical components:

1.  Certificate Authority (CA)
2.  Chat Server
3.  Chat Client(s)

Docker runs each component inside its own container and provides a
virtual network so that the containers can communicate with one another.

The architecture looks like this:

```text
                         Docker bridge network
                net-sec-chat-app_chat-network
                              │
          ┌───────────────────┼───────────────────┐
          │                   │                   │
          ▼                   ▼                   ▼
   ┌─────────────┐     ┌─────────────┐     ┌─────────────┐
   │     CA      │     │   SERVER    │     │   CLIENT    │
   │             │     │             │     │             │
   │ cert-auth   │     │ chat server │     │ chat client │
   │ :8081       │     │ :8080       │     │             │
   └─────────────┘     └─────────────┘     └─────────────┘
          ▲                   ▲                   │
          │                   │                   │
          └───────────────────┴───────────────────┘
```

Multiple client containers can exist simultaneously:

```text
                         Docker network
                              │
          ┌───────────────────┼───────────────────────┐
          │                   │                       │
          ▼                   ▼                       ▼
      ┌───────┐           ┌───────┐             ┌─────────┐
      │  CA   │           │Server │             │ Client1 │
      │ :8081 │           │ :8080 │             └─────────┘
      └───────┘           └───────┘                   │
          ▲                   ▲                       │
          │                   │                       │
          │                   └───────────────┐       │
          │                                   │       │
          │                                   ▼       ▼
          │                              ┌─────────┐
          └──────────────────────────────│ Client2 │
                                         └─────────┘
```

Each client is a separate container, but all clients communicate with
the same server.

---

# 2. Why Docker is being used

Without Docker, everything could be run directly on the host machine:

```text
Your computer
│
├── CA
├── Server
├── Client 1
├── Client 2
├── OpenSSL
└── networking
```

Docker instead gives us isolated environments:

```text
Your computer
│
└── Docker
    │
    ├── CA container
    ├── Server container
    ├── Client container
    └── Client container
```

The programs are still ordinary C programs compiled with GCC.

Docker provides the environment around those programs:

- filesystem isolation
- process isolation
- network isolation
- dependency installation
- reproducible environments
- controlled communication between components

This makes the architecture resemble a small distributed system.

---

# 3. Docker images and containers

Two important concepts are **images** and **containers**.

## Image

An image is a packaged template used to create containers.

This project has three images:

```text
chat-ca
chat-server
chat-client
```

For example, the client image is built from `Dockerfile.client`.

```dockerfile
FROM ubuntu:24.04
RUN apt-get update && apt-get install -y gcc libssl-dev
WORKDIR /app
COPY client /app/client
RUN mkdir -p client-key
RUN gcc client/client.c client/dh.c client/aes.c client/services.c client/cert.c -o client/client -lcrypto
CMD ["./client/client"]
```

This tells Docker to:

1.  Start with Ubuntu 24.04.
2.  Install GCC and OpenSSL development libraries.
3.  Set `/app` as the working directory.
4.  Copy the client source code into the image.
5.  Create the directory used for client keys.
6.  Compile the C program.
7.  Run `./client/client` when the container starts.

The resulting image is:

```text
chat-client
```

## Container

A container is a running instance of an image.

For example:

```text
chat-client image
       │
       ├── client container 1
       ├── client container 2
       └── client container 3
```

All three containers can be created from the same image, but each is a
separate running environment.

This is particularly useful for the chat application because each client
can represent a different user.

---

# 4. The Certificate Authority container

The CA is responsible for certificate-related operations.

Its container runs:

```text
./ca
```

The CA image is built using:

```dockerfile
FROM ubuntu:24.04
RUN apt-get update && apt-get install -y gcc libssl-dev
WORKDIR /app/cert-auth
COPY cert-auth .
RUN mkdir -p cert-key/users
RUN gcc cert-auth.c services.c -o ca -lcrypto
EXPOSE 8081
CMD ["./ca"]
```

The CA listens on:

```text
8081
```

Its responsibilities include things such as:

- generating the root CA key
- generating the root CA certificate
- receiving certificate signing requests
- signing certificates
- storing user certificates
- returning certificates to clients/server
- removing certificates when users leave

Conceptually:

```text
                  ┌─────────────────┐
                  │       CA        │
                  │                 │
                  │ Root CA key     │
                  │ Root CA cert    │
                  │ User certs      │
                  └────────┬────────┘
                           │
                    certificate
                    operations
                           │
              ┌────────────┴────────────┐
              │                         │
              ▼                         ▼
           Server                    Clients
```

---

# 5. The server container

The server image contains the chat server and its supporting C modules.

The server is responsible for:

- accepting client connections
- performing the initial authentication process
- communicating with the CA
- performing Diffie-Hellman key exchange
- deriving AES keys
- registering users
- maintaining connected users
- routing chat messages
- handling commands such as `/who` and `/quit`

The server listens on:

```text
8080
```

The server Dockerfile exposes:

```dockerfile
EXPOSE 8080
```

The Compose configuration additionally contains:

```yaml
ports:
  - "8080:8080"
```

This maps the host's port 8080 to the container's port 8080.

However, this mapping is not necessary for container-to-container
communication.

Inside the Docker network, the clients communicate directly with:

```text
server:8080
```

---

# 6. The client containers

A client is created from the `chat-client` image.

The command:

```bash
docker compose run --rm client ./client/client server 8080
```

creates a temporary client container.

The important parts are:

```text
client
```

which identifies the Compose service,

and:

```text
./client/client server 8080
```

which is the command executed inside the container.

The client therefore connects to:

```text
server:8080
```

rather than:

```text
localhost:8080
```

This distinction is extremely important.

Inside the client container:

```text
localhost
```

means:

> the client container itself.

It does NOT mean the server container.

`server` means:

> the Docker service/container named `server`.

Docker's internal DNS resolves that name to the server container's IP
address.

---

# 7. Docker Compose

Docker Compose is being used to describe the complete multi-container
application.

The Compose file contains:

```yaml
services:
  cert-auth:
    image: chat-ca
    container_name: cert-auth
    networks:
      - chat-network
    stdin_open: true
    tty: true

  server:
    image: chat-server
    container_name: server
    ports:
      - "8080:8080"
    networks:
      - chat-network
    stdin_open: true
    tty: true
    depends_on:
      - cert-auth

  client:
    image: chat-client
    networks:
      - chat-network
    stdin_open: true
    tty: true
    depends_on:
      - server
      - cert-auth

networks:
  chat-network:
    driver: bridge
```

Compose gives us a convenient way to define:

- which images are used
- which containers exist
- networking
- port mappings
- startup dependencies
- interactive terminal behavior

Instead of manually creating every Docker network and container, Compose
handles this configuration.

---

# 8. The Docker bridge network

The most important networking concept in this project is the custom
Docker bridge network:

```yaml
networks:
  chat-network:
    driver: bridge
```

Docker creates a virtual network for the services.

The actual Compose-generated network has a name similar to:

```text
net-sec-chat-app_chat-network
```

The containers attached to this network can communicate with one
another.

Conceptually:

```text
             Virtual Ethernet Network
                     │
        ┌────────────┼────────────┐
        │            │            │
        ▼            ▼            ▼
      CA          Server       Client
    :8081          :8080
```

Docker gives each container a virtual network interface and an IP
address.

You don't need to manually configure those IP addresses.

---

# 9. Docker's internal DNS

One of the nicest features being used here is Docker's internal DNS.

The server is called:

```text
server
```

and the CA is called:

```text
cert-auth
```

Therefore, from another container on the same Compose network:

```text
server
```

resolves to the server's container IP.

Likewise:

```text
cert-auth
```

resolves to the CA container's IP.

So the client can do:

```c
gethostbyname("server");
```

and connect to:

```text
server:8080
```

The server can similarly connect to:

```text
cert-auth:8081
```

This is much better than hard-coding Docker IP addresses because
container IPs can change.

The logical addressing becomes:

```text
Client ──> server:8080
Server ──> cert-auth:8081
```

rather than:

```text
Client ──> 172.x.x.x:8080
Server ──> 172.x.x.x:8081
```

---

# 10. `localhost` vs service names

This is worth emphasizing.

Suppose the server container has:

```text
172.20.0.3
```

and the client container has:

```text
172.20.0.4
```

Inside the client:

```text
localhost
```

means:

```text
172.20.0.4
```

the client itself.

It does not mean:

```text
172.20.0.3
```

the server.

Instead:

```text
server
```

is resolved by Docker DNS:

```text
server
  ↓
172.20.0.3
```

So:

```text
connect("server", 8080)
```

means:

```text
Client container
      │
      │ TCP
      ▼
Docker network
      │
      ▼
Server container :8080
```

---

# 11. Why clients are temporary containers

The Compose file defines one `client` service, but that does not mean
only one client can exist.

You run:

```bash
docker compose run --rm client ./client/client server 8080
```

Every time you execute this command, Docker creates a new container from
the `chat-client` image.

For example:

```text
chat-client image
       │
       ├── client container A
       ├── client container B
       └── client container C
```

All three connect to:

```text
server:8080
```

This lets you simulate multiple users.

For example:

```text
Terminal 1:
docker compose run --rm client ./client/client server 8080

Terminal 2:
docker compose run --rm client ./client/client server 8080

Terminal 3:
docker compose run --rm client ./client/client server 8080
```

You effectively get:

```text
                  ┌──────────┐
                  │  Server  │
                  │  :8080   │
                  └────┬─────┘
                       │
          ┌────────────┼────────────┐
          │            │            │
          ▼            ▼            ▼
       Client 1     Client 2     Client 3
```

This is a very useful way of testing your chat server.

---

# 12. Why `--rm` is used

The command:

```bash
docker compose run --rm client ./client/client server 8080
```

contains:

```text
--rm
```

This means:

> Remove the client container after the process exits.

So if a client quits:

```text
Client container
      │
      │ exits
      ▼
container removed
```

The image remains:

```text
chat-client
```

so you can create another client immediately.

This keeps the Docker environment clean.

---

# 13. What `depends_on` does

The Compose file contains:

```yaml
server:
  depends_on:
    - cert-auth
```

and:

```yaml
client:
  depends_on:
    - server
    - cert-auth
```

This expresses startup dependencies.

Conceptually:

```text
CA
 │
 ▼
Server
 │
 ▼
Client
```

It tells Compose about the intended startup order.

It does not, however, mean that the application is guaranteed to be
fully ready to accept connections. `depends_on` is primarily about
container startup ordering, not application-level readiness.

In your setup, this is generally fine because the CA and server are
long-running processes that start listening once initialized.

---

# 14. The complete startup sequence

When you run:

```bash
docker compose up cert-auth server
```

Compose starts the required containers.

You get:

```text
Docker
│
├── cert-auth
│      └── ./ca
│           └── listening on 8081
│
└── server
       └── ./server/server
            └── listening on 8080
```

Then you run:

```bash
docker compose run --rm client ./client/client server 8080
```

Docker creates a client container:

```text
client
  │
  └── ./client/client server 8080
```

The client resolves:

```text
server
```

through Docker DNS and establishes:

```text
Client ───────── TCP ─────────> Server:8080
```

---

# 15. Server ↔ CA communication

The server also needs to communicate with the CA.

For example, when obtaining a server certificate:

```text
Server
   │
   │ TCP
   │
   ▼
cert-auth:8081
```

The server sends a request and CSR to the CA.

Conceptually:

```text
Server
  │
  │ "I need a certificate"
  │
  │ CSR
  ▼
 CA
  │
  │ signs CSR
  ▼
Server certificate
```

The CA sends the resulting certificate back to the server.

The server can then use that certificate to prove its identity to
clients.

---

# 16. The certificate/authentication architecture

Your security architecture is roughly:

```text
                   Root of Trust
                        │
                        ▼
                ┌───────────────┐
                │      CA       │
                │               │
                │ CA private key│
                │ CA certificate│
                └───────┬───────┘
                        │
                  signs certificates
                        │
             ┌──────────┴──────────┐
             │                     │
             ▼                     ▼
       Server certificate     User certificates
             │
             ▼
          Server
             │
             │ proves possession
             │ of private key
             ▼
          Client
```

The CA therefore acts as the trust anchor.

The server has:

```text
server-key/server.key
server-key/server.crt
```

The CA has its own private key and certificate.

---

# 17. Network architecture vs cryptographic architecture

There are actually two architectures operating at the same time.

## Network architecture

Docker gives you:

```text
Client
   │
   │ TCP
   ▼
Server
   │
   │ TCP
   ▼
CA
```

## Cryptographic architecture

Your application provides:

```text
CA
 │
 │ certificates
 ▼
Server
 │
 │ certificate authentication
 │
 │ Diffie-Hellman
 ▼
Client
 │
 │ shared secret
 ▼
AES-GCM encrypted communication
```

So Docker is providing the **infrastructure/network environment**, while
OpenSSL and your C code implement the **security architecture**.

---

# 18. TCP framing inside the containers

Docker doesn't change how TCP works.

TCP is still a byte stream.

For example, if the server sends:

```text
HELLO
```

then the client cannot assume one `read()` returns all five bytes.

This is why you introduced:

```c
read_all()
write_all()
```

and length-prefix framing.

The application protocol now looks like:

```text
┌──────────────┬──────────────────────────────┐
│ 4-byte length│       message bytes          │
└──────────────┴──────────────────────────────┘
```

For example:

```text
[00 00 00 20][32 bytes of data]
```

The receiver first reads the length:

```text
N = 32
```

and then reads exactly 32 bytes.

This works regardless of whether the communication is:

```text
Client → Server
```

or:

```text
Server → CA
```

or:

```text
Server → Client
```

Docker simply provides the network path.

---

# 19. What Docker does NOT do

An important distinction is that Docker is not providing the
application's security.

Docker does not automatically:

- encrypt your TCP traffic
- perform Diffie-Hellman
- generate AES keys
- validate certificates
- authenticate clients
- implement AES-GCM
- provide your CA
- frame your messages

Those are implemented by your application.

For example:

```text
Docker
   │
   ├── creates containers
   ├── creates network
   ├── provides DNS
   └── provides process/filesystem isolation
```

Your C/OpenSSL application:

```text
Application
   │
   ├── TCP sockets
   ├── certificates
   ├── CA
   ├── Diffie-Hellman
   ├── AES-GCM
   ├── authentication
   └── chat protocol
```

This separation is important.

---

# 20. Why this is a distributed architecture

Even though everything is running on your laptop, the components behave
like separate machines.

You have:

```text
Machine-like environment 1
        CA

Machine-like environment 2
        Server

Machine-like environment 3
        Client
```

They communicate through actual TCP sockets.

The server doesn't directly call a CA function such as:

```c
generate_ca_certificate();
```

Instead, it sends a request over a socket:

```text
Server
   │
   │ TCP request
   ▼
CA
```

Similarly, the client doesn't directly access the server's memory.

It communicates over:

```text
Client
   │
   │ TCP
   ▼
Server
```

This makes the architecture much closer to a real deployment.

---

# 21. The role of the host machine

Your actual computer sits outside all of this.

Conceptually:

```text
                 Your physical machine
                         │
                       Docker
                         │
        ┌────────────────┼─────────────────┐
        │                │                 │
        ▼                ▼                 ▼
       CA              Server           Client
      :8081             :8080
```

The host can optionally access the server because of:

```yaml
ports:
  - "8080:8080"
```

This means:

```text
Host port 8080
      │
      ▼
Container port 8080
```

But the client does not need this mapping.

The client uses:

```text
server:8080
```

directly through the Docker network.

---

# 22. Why `EXPOSE` and `ports` are different

Your server Dockerfile has:

```dockerfile
EXPOSE 8080
```

This is primarily documentation/metadata saying:

> This container expects to use port 8080.

It does not itself publish the port to the host.

The Compose file has:

```yaml
ports:
  - "8080:8080"
```

This actually creates a host-to-container port mapping.

So:

```text
EXPOSE 8080
```

means:

```text
Container uses 8080
```

while:

```yaml
ports:
  - "8080:8080"
```

means:

```text
Host:8080 ─────> Container:8080
```

Container-to-container traffic doesn't require this host mapping.

---

# 23. Why the server listens on `INADDR_ANY`

Your server uses:

```c
server_addr.sin_addr.s_addr = INADDR_ANY;
```

This means the server listens on the container's available network
interfaces rather than only on a particular address.

That's important inside Docker.

The server needs to accept connections arriving through its Docker
network interface.

Conceptually:

```text
Docker network
      │
      ▼
Server container
      │
      ▼
INADDR_ANY
      │
      ▼
listen on :8080
```

If the server only listened on an inappropriate local interface, other
containers might not be able to connect.

---

# 24. Why this architecture is useful for your assignment

Your current design gives you a clean separation of responsibilities.

### CA

```text
Certificate authority
        │
        ├── CA key
        ├── CA certificate
        └── certificate issuance
```

### Server

```text
Chat server
        │
        ├── client connections
        ├── authentication
        ├── DH
        ├── AES keys
        ├── user management
        └── message routing
```

### Client

```text
Chat client
        │
        ├── server connection
        ├── certificate verification
        ├── DH
        ├── AES encryption/decryption
        └── user interaction
```

### Docker

```text
Infrastructure
        │
        ├── isolated environments
        ├── virtual network
        ├── DNS/service discovery
        └── reproducible deployment
```

Each layer has a relatively clear responsibility.

---

# 25. The architecture in one diagram

Putting everything together:

```text
                              HOST MACHINE
                                   │
                                 Docker
                                   │
                 ┌─────────────────┴─────────────────┐
                 │       Docker bridge network        │
                 │                                    │
                 │  net-sec-chat-app_chat-network     │
                 │                                    │
                 │   ┌─────────────┐                  │
                 │   │     CA      │                  │
                 │   │ cert-auth   │                  │
                 │   │    :8081   │                  │
                 │   └──────┬──────┘                  │
                 │          │                         │
                 │          │ TCP                     │
                 │          │ certificate requests   │
                 │          │                         │
                 │   ┌──────▼──────┐                  │
                 │   │   SERVER    │                  │
                 │   │    :8080    │                  │
                 │   └──────┬──────┘                  │
                 │          │                         │
                 │          │ TCP                     │
                 │          │ DH / authentication     │
                 │          │ encrypted chat          │
                 │          │                         │
                 │     ┌────┴────┬────────┐            │
                 │     │         │        │            │
                 │     ▼         ▼        ▼            │
                 │  Client 1  Client 2  Client 3     │
                 │                                    │
                 └────────────────────────────────────┘
```

And the logical security flow is:

```text
                    CA
                    │
                    │ signs
                    ▼
                 Server
                    │
                    │ certificate
                    ▼
                 Client
                    │
                    │ authenticate
                    ▼
             Diffie-Hellman
                    │
                    ▼
             Shared secret
                    │
                    ▼
              AES-GCM key
                    │
                    ▼
             Encrypted chat
```

---

# 26. The key idea

The easiest way to think about your entire setup is:

> **Docker creates the miniature network; your C programs implement the
> network protocol; OpenSSL implements the cryptography.**

So you have three layers:

```text
┌──────────────────────────────────────────┐
│          Application / Security         │
│                                          │
│ CA, certificates, DH, AES-GCM, chat     │
├──────────────────────────────────────────┤
│              TCP / Sockets               │
│                                          │
│ read(), write(), connect(), listen()    │
├──────────────────────────────────────────┤
│                 Docker                   │
│                                          │
│ containers, bridge network, DNS         │
└──────────────────────────────────────────┘
```

That is the architecture you're building: **a containerized,
multi-service, TCP client-server chat application with a dedicated CA
and application-level cryptographic security.**
