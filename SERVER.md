# Chat Server

NOTE: THIS FILE WAS BUILT USING CHATGPT.

This document explains how the chat server works, how multiple clients are handled, and how messages are forwarded between users.

The server is written in C and uses TCP sockets and `select()` to handle multiple clients without using `fork()`.

## 1. Basic Architecture

The server acts as a middleman between clients.

```text
Alice
  |
  | "hello"
  v
Server
  |
  | "alice: hello"
  v
Bob
```

The clients do not directly communicate with each other. The server handles all message forwarding.

## 2. Important Sockets

When the server starts, it creates one listening socket:

```c
s = socket(AF_INET, SOCK_STREAM, 0);
```

For example:

```text
Server listening socket -> 3

Alice   -> socket 4
Bob     -> socket 5
Mallory -> socket 6
```

The listening socket accepts new connections. Each accepted client gets its own socket.

## 3. User Structure

Every connected user is stored in:

```c
struct User
{
    char username[MAX_USERNAME];
    int socket;
    char chat_with[MAX_USERNAME];
};
```

For example:

```text
users[0]

username  = "alice"
socket    = 4
chat_with = "bob"
```

This means Alice is connected through socket 4 and is currently talking to Bob.

## 4. User Array

The server has:

```c
struct User users[MAX_USERS];
```

This can store up to 1000 users.

The number of currently connected users is stored in:

```c
int user_count;
```

For example:

```text
user_count = 3

users[0] -> alice
users[1] -> bob
users[2] -> mallory
```

## 5. Server Startup

The server does:

```text
socket()
   |
   v
bind()
   |
   v
listen()
   |
   v
wait for clients
```

The server listens on port `8080`.

## 6. Multiple Clients with select()

The server does not use `fork()` for every client.

Instead, it uses:

```c
select()
```

`select()` allows the server to watch multiple sockets at the same time.

For example:

```text
Listening socket -> 3

Alice   -> 4
Bob     -> 5
Mallory -> 6
```

The server asks `select()` which sockets are ready to be read.

This means the server does not get stuck waiting for one particular client.

## 7. fd_set

The server creates:

```c
fd_set readfds;
```

This is a collection of file descriptors that the server wants `select()` to watch.

First:

```c
FD_ZERO(&readfds);
```

clears the collection.

Then:

```c
FD_SET(s, &readfds);
```

adds the listening socket.

Every connected client's socket is also added:

```c
FD_SET(users[i].socket, &readfds);
```

For example:

```text
3 4 5 6

3 -> listening socket
4 -> Alice
5 -> Bob
6 -> Mallory
```

## 8. select()

The server calls:

```c
select(max_fd + 1,
       &readfds,
       NULL,
       NULL,
       NULL);
```

This waits until one or more of the watched sockets becomes ready.

For example, if Bob sends:

```text
hello
```

`select()` tells the server that Bob's socket is ready.

The server can then read Bob's data.

The server does not sit waiting on Alice while Bob is sending a message.

## 9. New Client Connection

The listening socket is also included in `select()`.

If:

```c
FD_ISSET(s, &readfds)
```

is true, a new client is waiting to connect.

The server does:

```c
c = accept(s, NULL, NULL);
```

Then:

```c
register_client(c);
```

registers the new client.

## 10. Client Registration

The server asks:

```text
Enter Name:
```

The client enters a username.

The server checks every existing user.

If the name already exists:

```text
Username already taken. Please choose another name.
```

The server asks again.

If the name is available, it stores:

```text
username
socket
chat_with
```

For example:

```text
alice
socket = 4
chat_with = ""
```

An empty `chat_with` means Alice has not selected anyone yet.

## 11. /who

The command:

```text
/who
```

asks the server for all currently connected users.

The server loops through `users[]` and sends their usernames back.

Example:

```text
/who

1.    alice
2.    bob
3.    mallory
```

## 12. /chat <username>

The command:

```text
/chat bob
```

selects Bob as the current chat target.

The server manually extracts the username. Since `/chat ` is six characters long, the username starts at `buf[6]`.

The server then searches for Bob using:

```c
find_user("bob");
```

If Bob exists:

```text
alice.chat_with = "bob"
```

The client receives:

```text
Now chatting with bob
```

If Bob does not exist:

```text
User bob not found.
```

## 13. Normal Messages

After Alice selects Bob:

```text
/chat bob
```

Alice can type:

```text
hello
```

The server checks Alice's `chat_with` value and sees:

```text
bob
```

It searches `users[]` for Bob, gets Bob's socket, and sends:

```text
alice: hello
```

to Bob.

The flow is:

```text
Alice
  |
  | hello
  v
Server
  |
  | find Alice's chat_with
  | find Bob in users[]
  | get Bob's socket
  v
Bob
```

## 14. @username message

The server also supports:

```text
@bob hello
```

The server first extracts:

```text
bob
```

using `get_username()`.

Then:

```c
service_chat_username(i, username);
```

changes Alice's:

```text
chat_with
```

to:

```text
bob
```

The server then moves the message pointer past `@bob ` so only:

```text
hello
```

is forwarded.

Bob receives:

```text
alice: hello
```

This means `@bob hello` both selects Bob and sends the first message.

## 15. /quit

The command:

```text
/quit
```

removes the user from the server.

The server:

1. Closes the user's socket.
2. Removes the user from `users[]`.
3. Decreases `user_count`.
4. Shifts later users down in the array.

For example:

```text
Before:

alice
bob
mallory

Alice quits.

After:

bob
mallory
```

## 16. Client Disconnection

A client can also disconnect without sending `/quit`.

When:

```c
read(client_socket, buf, ...);
```

returns `0` or a negative value, the server treats the client as disconnected.

It calls:

```c
service_quit();
```

to remove that user.

## 17. Main Server Flow

The main server loop is essentially:

```text
START SERVER
    |
    v
Create listening socket
    |
    v
Bind to port 8080
    |
    v
Listen
    |
    v
Build fd_set
    |
    v
select()
    |
    v
Something is ready
    |
    +-------------------+
    |                   |
    v                   v
New connection?      Client data?
    |                   |
    v                   v
 accept()              read()
    |                   |
    v                   v
register client      process command
    |                   |
    +---------+---------+
              |
              v
        Repeat forever
```

## 18. Command Flow

The server handles commands like this:

```text
/who
  |
  v
service_who()


/chat bob
  |
  v
service_chat()


/quit
  |
  v
service_quit()


@bob hello
  |
  +--> get_username()
  |
  +--> service_chat_username()
  |
  +--> service_message()


normal message
  |
  v
service_message()
```

## 19. Overall Architecture

The final architecture is:

```text
                    SERVER
                      |
              listening socket
                      |
              +-------+-------+
              |       |       |
            Alice    Bob    Mallory
            fd 4     fd 5     fd 6
              |       |       |
              +-------+-------+
                      |
                   select()
                      |
              Server processes
                  messages
                      |
              +-------+-------+
              |               |
           Alice -> Bob    Bob -> Alice
```

The important point is that the server owns all client sockets.

Clients do not directly connect to each other.

The server is the central proxy:

```text
Client A -> Server -> Client B
Client B -> Server -> Client A
```

This design allows the server to handle many connected clients in one process using `select()` instead of creating a separate process with `fork()`.
