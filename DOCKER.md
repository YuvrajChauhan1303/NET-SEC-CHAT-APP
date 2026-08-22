# Docker Setup for NET-SEC-CHAT-APP

NOTE: THIS FILE WAS BUILT USING CHATGPT.

## 1. What are we trying to build?

Our project is a TCP chat application written in C.

So far, the application has:

- A TCP server.
- Multiple TCP clients.
- Username registration.
- `/quit` functionality.
- `/who` functionality.
- `fork()` for handling multiple clients.
- Shared memory using `mmap()` so forked server processes can share the list of users.

We now want to run the application as separate virtual networked machines.

Instead of creating four full Ubuntu virtual machines with VirtualBox, we are using **Docker containers**.

Our target setup is:

```text
                         Docker Network
                 net-sec-chat-app_chat-network
                              |
             +----------------+----------------+
             |                |                |
          SERVER           CLIENT 1         CLIENT 2
          :8080             alice              bob
             |
             |
          CLIENT 3
          mallory
```

More accurately, all four containers are connected to the same Docker bridge network:

```text
server   -> 172.18.0.2
client1  -> 172.18.0.3
client2  -> 172.18.0.4
client3  -> 172.18.0.5
```

The IP addresses are assigned by Docker and may change when the containers are recreated. Therefore, clients should **not** hardcode `172.18.0.2`.

Instead, clients connect to:

```text
server:8080
```

Docker's internal DNS resolves `server` to the current IP address of the server container.

---

# 2. Why Docker instead of VirtualBox?

We could create four complete Ubuntu virtual machines:

```text
Ubuntu VM 1 -> Server
Ubuntu VM 2 -> Client 1
Ubuntu VM 3 -> Client 2
Ubuntu VM 4 -> Client 3
```

But that is unnecessarily heavy for this project.

Docker gives us:

```text
1 server container
3 client containers
1 Docker network
```

Containers are much lighter than full virtual machines.

For this project, each container behaves like an independent Linux machine from the point of view of our TCP application.

This is especially useful later when we analyze the network traffic with Wireshark.

---

# 3. Our environment

We are developing on:

```text
Windows
   |
   +-- WSL2
   |     |
   |     +-- Ubuntu 24.04
   |
   +-- Docker Desktop
          |
          +-- Docker Engine
```

The project itself is stored inside the WSL filesystem:

```text
~/NET-SEC-CHAT-APP
```

This is preferable to keeping the active Linux project under `/mnt/c/...` because our program is a Linux/POSIX C application.

We can still open the project using Windows VS Code through WSL integration:

```bash
cd ~/NET-SEC-CHAT-APP
code .
```

---

# 4. Important Docker concepts

## 4.1 Image

A Docker image is a template from which containers are created.

For example:

```text
chat-server image
       |
       +---- server container
```

and:

```text
chat-client image
       |
       +---- client1 container
       +---- client2 container
       +---- client3 container
```

The same client image can therefore be used to create three independent client containers.

---

## 4.2 Container

A container is a running instance of an image.

Our containers are:

```text
server
client1
client2
client3
```

Each has its own process environment and network identity.

---

## 4.3 Dockerfile

A Dockerfile tells Docker how to build an image.

We have two:

```text
Dockerfile.server
Dockerfile.client
```

---

## 4.4 Docker Compose

Docker Compose lets us describe related containers and their network in one configuration file.

Our Compose file is:

```text
docker-compose.yml
```

At the moment, Compose manages the server. The clients are launched separately because our clients are interactive terminal programs that need their own stdin.

---

# 5. Project structure

Our project currently looks like:

```text
NET-SEC-CHAT-APP/
|
+-- Makefile
+-- README.md
+-- Dockerfile.server
+-- Dockerfile.client
+-- docker-compose.yml
|
+-- client/
|    +-- client.c
|
+-- server/
     +-- server.c
     +-- services.c
     +-- services.h
```

---

# 6. Building the project normally

Before introducing Docker, we verify that the C project works in normal WSL.

From the project root:

```bash
cd ~/NET-SEC-CHAT-APP
make
```

This currently produces:

```text
client/client
server/server
```

The normal build uses GCC:

```text
gcc client/client.c -o client/client
gcc server/server.c server/services.c -o server/server
```

This is important because if something breaks later, we can determine whether the problem is with our C program or Docker.

---

# 7. Server networking code

Our server uses:

```c
addr.sin_family = AF_INET;
addr.sin_port = htons(8080);
addr.sin_addr.s_addr = INADDR_ANY;
```

The important part is:

```c
INADDR_ANY
```

This means:

> Listen for connections on all available network interfaces.

This is important inside Docker.

We do NOT want the server to listen only on:

```text
127.0.0.1
```

because that would mean it only accepts connections from inside the same container.

The server listens on:

```text
0.0.0.0:8080
```

which allows connections through its Docker network interface.

---

# 8. Why 127.0.0.1 caused a problem

Originally the client contained:

```c
inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
```

This works when both programs are on the same machine.

But Docker containers have separate network namespaces.

Inside `client1`:

```text
127.0.0.1
```

means:

```text
client1 itself
```

It does NOT mean:

```text
server container
```

Therefore this would be wrong inside Docker:

```text
client1 -> 127.0.0.1:8080
```

Instead, we use:

```text
client1 -> server:8080
```

---

# 9. Client hostname support

The client was changed to use `getaddrinfo()`.

The important part of the client is now:

```c
#include <netdb.h>
```

and:

```c
int main(int argc, char *argv[])
```

We use default values:

```c
char *host = "127.0.0.1";
char *port = "8080";
```

Then command-line arguments can override them:

```c
if (argc >= 2)
    host = argv[1];

if (argc >= 3)
    port = argv[2];
```

Then:

```c
struct addrinfo hints, *res;

memset(&hints, 0, sizeof(hints));
hints.ai_family = AF_INET;
hints.ai_socktype = SOCK_STREAM;

getaddrinfo(host, port, &hints, &res);

s = socket(res->ai_family,
           res->ai_socktype,
           res->ai_protocol);

connect(s, res->ai_addr, res->ai_addrlen);

freeaddrinfo(res);
```

This allows both:

```bash
./client/client
```

and:

```bash
./client/client server 8080
```

The first means:

```text
127.0.0.1:8080
```

The second means:

```text
server:8080
```

This makes the same client program usable both locally and inside Docker.

---

# 10. Server Dockerfile

Our `Dockerfile.server` is:

```dockerfile
FROM ubuntu:24.04

RUN apt update && apt install -y gcc

WORKDIR /app

COPY server/ /app/server/

RUN gcc server/server.c server/services.c -o server/server

CMD ["./server/server"]
```

## Explanation

### Start with Ubuntu

```dockerfile
FROM ubuntu:24.04
```

The container starts with a basic Ubuntu 24.04 environment.

### Install GCC

```dockerfile
RUN apt update && apt install -y gcc
```

We need GCC because the server is written in C.

### Set working directory

```dockerfile
WORKDIR /app
```

Inside the container, `/app` becomes our working directory.

### Copy source code

```dockerfile
COPY server/ /app/server/
```

This copies our local `server/` directory into the image.

### Compile

```dockerfile
RUN gcc server/server.c server/services.c -o server/server
```

The server is compiled inside the Docker image.

### Start server

```dockerfile
CMD ["./server/server"]
```

When the container starts, this executable is run.

---

# 11. Client Dockerfile

Our `Dockerfile.client` is:

```dockerfile
FROM ubuntu:24.04

RUN apt update && apt install -y gcc

WORKDIR /app

COPY client/ /app/client/

RUN gcc client/client.c -o client/client

CMD ["./client/client", "server", "8080"]
```

The important difference is:

```dockerfile
CMD ["./client/client", "server", "8080"]
```

So when the client container starts, it tries to connect to:

```text
server:8080
```

Docker provides the DNS resolution for `server`.

---

# 12. Building the Docker images

From:

```bash
cd ~/NET-SEC-CHAT-APP
```

build the server image:

```bash
docker build -f Dockerfile.server -t chat-server .
```

Build the client image:

```bash
docker build -f Dockerfile.client -t chat-client .
```

We can verify them with:

```bash
docker images
```

We should see:

```text
chat-server
chat-client
```

---

# 13. Docker Compose configuration

Our current `docker-compose.yml` is:

```yaml
services:
  server:
    image: chat-server
    container_name: server
    ports:
      - "8080:8080"
    networks:
      - chat-network
    stdin_open: true
    tty: true

networks:
  chat-network:
    driver: bridge
```

## Server service

```yaml
server:
```

This defines our server service.

```yaml
image: chat-server
```

Use the Docker image we built earlier.

```yaml
container_name: server
```

Give the container a predictable name.

This is important because clients can use:

```text
server
```

as the hostname.

```yaml
ports:
  - "8080:8080"
```

This maps:

```text
host port 8080
       |
       v
container port 8080
```

This is useful for connecting from the WSL/host environment.

The clients themselves do NOT need this mapping to communicate with the server because they use the Docker network directly.

```yaml
networks:
  - chat-network
```

Attach the server to the Compose network.

```yaml
stdin_open: true
tty: true
```

Our server currently contains a `fgets()` loop, so it needs an interactive stdin/terminal.

---

# 14. Starting the server

From the project root:

```bash
docker compose up -d
```

Check it:

```bash
docker compose ps
```

We should see:

```text
NAME      IMAGE         COMMAND             SERVICE   STATUS
server    chat-server   "./server/server"   server    Up
```

To see server logs:

```bash
docker compose logs server
```

The server should print:

```text
Server Initialized. Listening for requests.
```

---

# 15. The Docker network

Compose automatically creates a network based on the project name.

Our network is currently:

```text
net-sec-chat-app_chat-network
```

It is a Docker bridge network.

We can inspect it with:

```bash
docker network inspect net-sec-chat-app_chat-network
```

The network currently uses:

```text
172.18.0.0/16
```

with gateway:

```text
172.18.0.1
```

---

# 16. Current working network topology

Our working setup has been verified as:

```text
server   -> 172.18.0.2
client1  -> 172.18.0.3
client2  -> 172.18.0.4
client3  -> 172.18.0.5
```

These addresses are examples from the current run and can change when containers are recreated.

The important thing is that all four containers belong to:

```text
net-sec-chat-app_chat-network
```

---

# 17. How Docker DNS works

Inside the Docker network, clients can use:

```text
server
```

as a hostname.

Docker's internal DNS translates:

```text
server
```

into the current IP address of the server container.

For example:

```text
client1
   |
   | getaddrinfo("server", "8080")
   |
   v
Docker DNS
   |
   v
172.18.0.2
   |
   v
server:8080
```

This is why we should NOT hardcode:

```text
172.18.0.2
```

in the C program.

---

# 18. Creating client containers

Each client should be its own container.

For client 1:

```bash
docker run --rm -it \
  --name client1 \
  --network net-sec-chat-app_chat-network \
  chat-client \
  ./client/client server 8080
```

For client 2:

```bash
docker run --rm -it \
  --name client2 \
  --network net-sec-chat-app_chat-network \
  chat-client \
  ./client/client server 8080
```

For client 3:

```bash
docker run --rm -it \
  --name client3 \
  --network net-sec-chat-app_chat-network \
  chat-client \
  ./client/client server 8080
```

Each command creates a separate container.

The `-it` options are important because our client is interactive and needs keyboard input.

---

# 19. Why clients are not currently started using `docker compose up`

We initially tried making `client1` a normal Compose service.

The problem was not container-to-container networking.

Container-to-container networking worked perfectly.

The problem was that our C client is an interactive terminal application.

It uses:

```c
fgets(buf, sizeof(buf), stdin);
```

to wait for user input.

When Compose started it as a normal service, its stdin behavior caused the client to exit.

Running:

```bash
docker run --rm -it ...
```

gives each client its own interactive terminal.

This matches what we actually want:

```text
Terminal 1 -> client1
Terminal 2 -> client2
Terminal 3 -> client3
```

Therefore, for now:

- Docker Compose manages the server.
- `docker run -it` creates each interactive client container.

This is still completely container-to-container communication.

---

# 20. Verified three-client setup

We tested the following:

```text
server
client1 -> alice
client2 -> bob
client3 -> mallory
```

All three clients connected to:

```text
server:8080
```

and the server reported:

```text
1.    alice
2.    bob
3.    mallory
```

This proves that:

1. Three separate client containers can connect to one server container.
2. The Docker network works.
3. Docker DNS works.
4. TCP communication works.
5. The server's forked processes work.
6. The server's shared `mmap()` state is visible to the different server child processes.
7. `/who` sees users connected from different containers.

---

# 21. What `127.0.0.1` means in Docker

This is one of the most important concepts to remember.

Inside client1:

```text
127.0.0.1
```

means:

```text
client1
```

Inside client2:

```text
127.0.0.1
```

means:

```text
client2
```

Inside the server:

```text
127.0.0.1
```

means:

```text
server
```

Therefore:

```text
client1 -> 127.0.0.1:8080
```

does NOT mean:

```text
client1 -> server:8080
```

Instead:

```text
client1 -> server:8080
```

is the correct container-to-container connection.

---

# 22. Host port vs container port

This is another important distinction.

Our Compose configuration contains:

```yaml
ports:
  - "8080:8080"
```

The format is:

```text
HOST_PORT:CONTAINER_PORT
```

Therefore:

```text
8080:8080
```

means:

```text
Host port 8080
      |
      v
Server container port 8080
```

This allows our WSL host to reach:

```text
127.0.0.1:8080
```

But client containers do not need to use this mapping.

They can directly use:

```text
server:8080
```

through the Docker network.

---

# 23. Useful Docker commands

## See running containers

```bash
docker ps
```

## See all containers

```bash
docker ps -a
```

## See Docker images

```bash
docker images
```

## See Compose services

```bash
docker compose ps
```

## Start server

```bash
docker compose up -d
```

## Stop server

```bash
docker compose down
```

## See server logs

```bash
docker compose logs server
```

## Inspect network

```bash
docker network inspect net-sec-chat-app_chat-network
```

## Start client 1

```bash
docker run --rm -it \
  --name client1 \
  --network net-sec-chat-app_chat-network \
  chat-client \
  ./client/client server 8080
```

## Start client 2

```bash
docker run --rm -it \
  --name client2 \
  --network net-sec-chat-app_chat-network \
  chat-client \
  ./client/client server 8080
```

## Start client 3

```bash
docker run --rm -it \
  --name client3 \
  --network net-sec-chat-app_chat-network \
  chat-client \
  ./client/client server 8080
```

---

# 24. Useful Docker networking test

We previously used a temporary Ubuntu container to test whether it could reach the server:

```bash
docker run --rm -it \
  --network net-sec-chat-app_chat-network \
  ubuntu:24.04 bash
```

Inside it, we installed netcat:

```bash
apt update && apt install -y netcat-openbsd
```

Then tested:

```bash
nc -vz server 8080
```

A successful result looks like:

```text
Connection to server 8080 port [tcp/*] succeeded!
```

This is a useful troubleshooting technique.

It tells us:

> The Docker network can reach the server's TCP port.

---

# 25. How the complete system works

When everything is running:

```text
                       Docker
                         |
              net-sec-chat-app_chat-network
                         |
        +----------------+----------------+
        |                |                |
        v                v                v
     server           client1          client2
  172.18.0.2        172.18.0.3        172.18.0.4
     :8080              |                |
        ^                |                |
        |                |                |
        +----------------+----------------+
                         |
                      client3
                    172.18.0.5
```

A client performs:

```text
getaddrinfo("server", "8080")
        |
        v
Docker DNS
        |
        v
172.18.0.2
        |
        v
TCP connection to port 8080
        |
        v
server
```

The server then handles the connection using `fork()`.

The server's shared user information is stored using:

```c
mmap(... MAP_SHARED | MAP_ANONYMOUS ...)
```

so its forked processes can access the shared user list.

---

# 26. Current application flow

For a new client:

```text
1. Client container starts.
2. Client resolves "server".
3. Client connects to server:8080.
4. Server accepts connection.
5. Server forks a child.
6. Server asks for username.
7. Client sends username.
8. Server adds username to shared users[].
9. Server sends registration response.
10. Client can send commands.
```

For `/who`:

```text
client
  |
  | "/who"
  v
server child
  |
  | service_who()
  v
shared users[]
  |
  v
server sends list
  |
  v
client
```

For `/quit`:

```text
client
  |
  | "/quit"
  v
server
  |
  | service_quit()
  v
remove user
  |
  v
close socket
```

---

# 27. Current limitations / things to remember

This Docker setup is working, but there are some application-level issues we should address later.

## TCP is a byte stream

TCP does not preserve message boundaries.

A single:

```c
write()
```

does not guarantee a corresponding single:

```c
read()
```

on the other side.

For a more robust chat protocol, we should eventually define message boundaries, for example using newline-terminated messages:

```text
hello\n
/who\n
/quit\n
@alice hello\n
```

and read complete lines.

---

## `write()` may not send everything

For larger messages, robust code should handle partial writes.

Eventually we can create a helper that keeps calling `write()` until the entire message is sent.

---

## `read()` may return partial messages

Similarly, robust receiving code should accumulate bytes until the complete application-level message has arrived.

---

## Server synchronization

The server currently shares:

```c
users[]
user_count
```

between forked processes.

Multiple processes can potentially modify this shared data at the same time.

Before scaling the chat system further, we should eventually consider process synchronization such as:

```text
semaphore
mutex in shared memory
file locking
```

depending on the requirements of the assignment.

---

# 28. Current status

At this point we have successfully completed the Docker networking portion:

```text
[x] Install Docker
[x] Configure Docker with WSL2
[x] Move project into WSL
[x] Build C project in WSL
[x] Create server Dockerfile
[x] Build server Docker image
[x] Create client Dockerfile
[x] Build client Docker image
[x] Create Docker Compose network
[x] Run server in Docker
[x] Create separate client containers
[x] Connect clients to server container
[x] Use Docker DNS
[x] Use server:8080
[x] Test two clients simultaneously
[x] Test three clients simultaneously
[x] Test /who across containers
```

The next major task is:

```text
[ ] Capture and analyze TCP traffic using Wireshark
```

We are deliberately postponing that until the Docker setup is understood and stable.

---

# 29. Mental model for the team

The easiest way to remember the whole setup is:

```text
IMAGE
  |
  | docker run
  v
CONTAINER
  |
  | attach to
  v
DOCKER NETWORK
  |
  +-------------------+
  |                   |
SERVER              CLIENT
  |                   |
  |<------ TCP -------|
```

For our project:

```text
chat-server image
       |
       v
server container
       |
       +----------------------+
                              |
                    Docker network
                              |
       +----------------------+----------------------+
       |                      |                      |
   client1                client2                client3
   alice                    bob                  mallory
```

The most important rule is:

> **Containers communicate using Docker network names such as `server`, not `127.0.0.1`.**

`127.0.0.1` means "this container."

`server` means "the server container."

---

# 30. Quick-start commands

After the setup is already built, the normal workflow is:

### Start server

```bash
cd ~/NET-SEC-CHAT-APP
docker compose up -d
```

### Start client 1

```bash
docker run --rm -it \
  --name client1 \
  --network net-sec-chat-app_chat-network \
  chat-client \
  ./client/client server 8080
```

### Start client 2

Open another terminal:

```bash
docker run --rm -it \
  --name client2 \
  --network net-sec-chat-app_chat-network \
  chat-client \
  ./client/client server 8080
```

### Start client 3

Open another terminal:

```bash
docker run --rm -it \
  --name client3 \
  --network net-sec-chat-app_chat-network \
  chat-client \
  ./client/client server 8080
```

### Stop server

```bash
docker compose down
```

Because the clients use:

```text
--rm
```

their containers are automatically removed when their client process exits.

---

# 31. Final architecture

The final architecture for this stage is:

```text
                         HOST / WSL2
                              |
                         Docker Engine
                              |
                  +-----------+-----------+
                  |                       |
             Docker Compose          docker run
                  |                       |
                  v                       v
             SERVER CONTAINER       CLIENT CONTAINERS
                  |                  +--- client1
                  |                  +--- client2
                  |                  +--- client3
                  |
                  +-------- Docker Network --------+
                              |
                    net-sec-chat-app_chat-network
                              |
                       TCP port 8080
```

This gives us a lightweight, reproducible environment with one server and three independent clients, while keeping the application itself as a normal POSIX C TCP program.
