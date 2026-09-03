# MITM Proxy

## Overview

The `mitm.c` program implements a **Man-in-the-Middle (MITM) proxy** for the encrypted chat application.

The proxy is placed between the client and the real server:

```text
Client
  |
  | TCP connection :8000
  |
  v
MITM Proxy
  |
  | TCP connection :8080
  |
  v
Real Server
```

The MITM proxy establishes **two separate TCP connections**:

1. A connection between the client and the MITM.
2. A connection between the MITM and the real server.

The MITM also performs **two independent Diffie-Hellman key exchanges**. Therefore, the client and server do not share the same AES key.

```text
                 MITM
              /        \
             /          \
        AES Key 1      AES Key 2
           /              \
       Client            Server
```

This allows the MITM to decrypt messages received from either side, print the plaintext, encrypt the message again using the key belonging to the other side, and forward it.

---

# Endpoints

There are two communication endpoints in the MITM proxy.

## Client Endpoint

The client connects to:

```text
127.0.0.1:8000
```

The MITM listens on port `8000`.

The listening socket is stored in:

```c
s
```

After a client connects, `accept()` creates another socket:

```c
c = accept(s, NULL, NULL);
```

The socket `c` represents the actual communication channel between the client and MITM.

```text
Client <------ c ------> MITM
```

The MITM uses:

```c
read(c, ...)
write(c, ...)
```

to receive and send data to the client.

---

## Server Endpoint

The real server is running on:

```text
127.0.0.1:8080
```

The MITM creates a socket:

```c
server_socket = socket(AF_INET, SOCK_STREAM, 0);
```

and connects it to the real server:

```c
connect(server_socket,
        (struct sockaddr *)&server_addr,
        sizeof(server_addr));
```

This creates the second communication channel:

```text
MITM <------ server_socket ------> Server
```

The MITM uses:

```c
read(server_socket, ...)
write(server_socket, ...)
```

to communicate with the real server.

---

# Socket Structure

The MITM program has three socket descriptors:

| Variable        | Purpose                            |
| --------------- | ---------------------------------- |
| `s`             | Listening socket on port 8000      |
| `c`             | Client ↔ MITM communication socket |
| `server_socket` | MITM ↔ Server communication socket |

The important distinction is that `s` is only used to accept connections.

```text
                    accept()
Client ------------------------------> MITM
                                      |
                                      | s
                                      | listening socket
                                      v
                                  accept(s)
                                      |
                                      v
                                      c
                                      |
                                      v
                                   Client
```

After the client connects, the actual communication happens through `c`.

---

# Diffie-Hellman Key Exchanges

The MITM performs two independent Diffie-Hellman exchanges.

## 1. Client-side DH Exchange

The function:

```c
dh_client(c, client_aes_key);
```

performs the first exchange.

The MITM generates its own private key:

```c
client_sec
```

and calculates its public share:

```c
client_share
```

The client sends its public DH value to the MITM.

The MITM then calculates:

```text
shared_secret_client
```

using the client's public value and the MITM's private value.

The shared secret is passed to:

```c
derive_aes_key(secret, client_aes_key);
```

This produces:

```text
client_aes_key
```

The client therefore shares an AES key with the MITM.

```text
Client
   |
   | DH exchange
   |
   v
MITM

Client AES Key = client_aes_key
```

---

# 2. Server-side DH Exchange

The function:

```c
dh_server(server_socket, server_aes_key);
```

performs a second, independent Diffie-Hellman exchange.

The MITM generates another private key and public share.

The MITM sends its fake client public share to the real server.

The real server responds with its DH public share.

The MITM calculates another shared secret:

```text
shared_secret_server
```

and derives:

```c
server_aes_key
```

using:

```c
derive_aes_key(secret, server_aes_key);
```

Therefore:

```text
MITM <------ AES ------> Server
```

uses a different AES key from:

```text
Client <------ AES ------> MITM
```

---

# Final Key Structure

After both DH exchanges, the MITM has two AES keys:

```text
client_aes_key
server_aes_key
```

The key relationships are:

```text
Client
   |
   | client_aes_key
   |
   v
 MITM
   |
   | server_aes_key
   |
   v
Server
```

The client does **not** have `server_aes_key`.

The server does **not** have `client_aes_key`.

---

# Message Flow

After the DH exchanges are complete, the MITM enters an infinite loop:

```c
while(1)
```

It uses `select()` to monitor both communication sockets:

```c
FD_SET(c, &readfds);
FD_SET(server_socket, &readfds);
```

This allows the MITM to detect which endpoint has sent data.

---

# Client → Server Message

When the client sends an encrypted message:

```c
if(FD_ISSET(c, &readfds))
```

the MITM reads the encrypted packet:

```c
read(c, encrypted, sizeof(encrypted));
```

The message is encrypted using `client_aes_key`.

Therefore, the MITM decrypts it using:

```c
decrypt_message(
    encrypted,
    n,
    client_aes_key,
    plaintext
);
```

The plaintext is then printed:

```c
printf("PLaintext from client: %s\n", plaintext);
```

This demonstrates that the MITM can read the client's message.

The MITM then encrypts the plaintext again using the server's key:

```c
encrypt_message(
    plaintext,
    plaintext_len,
    server_aes_key,
    encrypted_again
);
```

Finally, it sends the newly encrypted packet to the server:

```c
write(server_socket, encrypted_again, encrypted_len);
```

The complete flow is:

```text
Client
   |
   | Encrypted using client_aes_key
   v
 MITM
   |
   | Decrypt using client_aes_key
   v
 Plaintext
   |
   | Print plaintext
   |
   | Encrypt using server_aes_key
   v
 MITM
   |
   | Encrypted using server_aes_key
   v
Server
```

---

# Server → Client Message

The reverse process occurs when the server sends a message.

The MITM detects data on:

```c
server_socket
```

and reads the encrypted packet:

```c
read(server_socket, encrypted, sizeof(encrypted));
```

The server encrypted the message using `server_aes_key`.

Therefore, the MITM decrypts it using:

```c
decrypt_message(
    encrypted,
    n,
    server_aes_key,
    plaintext
);
```

The plaintext is printed:

```c
printf("PLaintext from Server: %s\n", plaintext);
```

The MITM then encrypts the plaintext using:

```c
client_aes_key
```

```c
encrypt_message(
    plaintext,
    plaintext_len,
    client_aes_key,
    encrypted_again
);
```

Finally, it sends the newly encrypted message to the client:

```c
write(c, encrypted_again, encrypted_len);
```

The flow is:

```text
Server
   |
   | Encrypted using server_aes_key
   v
 MITM
   |
   | Decrypt using server_aes_key
   v
 Plaintext
   |
   | Print plaintext
   |
   | Encrypt using client_aes_key
   v
 MITM
   |
   | Encrypted using client_aes_key
   v
Client
```

---

# `select()` and Communication

The MITM needs to listen to both sides at the same time.

It uses:

```c
select(max_fd + 1, &readfds, NULL, NULL, NULL);
```

The two communication sockets are added to the set:

```c
FD_SET(c, &readfds);
FD_SET(server_socket, &readfds);
```

The MITM then checks:

```c
FD_ISSET(c, &readfds)
```

to determine whether the client sent something.

It checks:

```c
FD_ISSET(server_socket, &readfds)
```

to determine whether the server sent something.

Therefore:

```text
                 MITM
              /        \
             /          \
        c /              \ server_socket
          /                \
     Client                Server
        ↑                    ↑
        |                    |
   select() monitors both sockets
```

---

# Important Functions

## `dh_client()`

```c
dh_client(c, client_aes_key);
```

Performs the Diffie-Hellman exchange between the client and MITM and generates `client_aes_key`.

---

## `dh_server()`

```c
dh_server(server_socket, server_aes_key);
```

Performs the Diffie-Hellman exchange between the MITM and real server and generates `server_aes_key`.

---

## `derive_aes_key()`

```c
derive_aes_key(secret, aes_key);
```

Converts the Diffie-Hellman shared secret into an AES key.

---

## `decrypt_message()`

```c
decrypt_message(encrypted, n, key, plaintext);
```

Decrypts an AES-GCM encrypted message.

The MITM uses different keys depending on which side sent the message.

---

## `encrypt_message()`

```c
encrypt_message(plaintext, plaintext_len, key, encrypted);
```

Encrypts plaintext using AES-GCM.

The MITM uses the **other side's key** when forwarding the message.

---

## `select()`

```c
select(max_fd + 1, &readfds, NULL, NULL, NULL);
```

Allows the MITM to monitor both the client and server sockets.

---

# Complete MITM Flow

The complete operation of the program is:

```text
                    START
                      |
                      v
             Create listening socket
                      |
                      v
             Listen on port 8000
                      |
                      v
                accept(client)
                      |
                      v
              Connect to server
                 port 8080
                      |
                      v
             Initialize DH parameters
                      |
          +-----------+-----------+
          |                       |
          v                       v
    DH with Client          DH with Server
          |                       |
          v                       v
 client_aes_key            server_aes_key
          |                       |
          +-----------+-----------+
                      |
                      v
                MITM Ready
                      |
                      v
                  select()
                 /       \
                /         \
               v           v
           Client         Server
              |              |
              v              v
           Decrypt        Decrypt
              |              |
              v              v
          Print data      Print data
              |              |
              v              v
           Encrypt        Encrypt
          with server     with client
              |              |
              v              v
           Server         Client
```

---

# Closing the Connections

When either side closes the connection, `read()` returns `0`:

```c
if(n <= 0){
    break;
}
```

The loop is then terminated.

The sockets are closed using:

```c
close(c);
close(server_socket);
```

The DH parameters are released:

```c
free_dh_params();
```

and the program exits.

---

# Purpose of the MITM

The purpose of this program is to demonstrate an important weakness of unauthenticated Diffie-Hellman.

Diffie-Hellman can establish a shared secret, but by itself it does **not authenticate who the other party is**.

The client believes it is performing DH with the server:

```text
Client <------ DH ------> Server
```

but the MITM actually intercepts the exchange:

```text
Client <------ DH ------> MITM <------ DH ------> Server
```

As a result, the MITM establishes two different shared secrets and can decrypt, inspect, and re-encrypt the communication.

This demonstrates why secure protocols such as TLS use **authentication mechanisms**, such as certificates and certificate authorities, in addition to key exchange.

