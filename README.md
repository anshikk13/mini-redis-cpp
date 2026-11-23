
# Mini Redis

I have built my own mini version of Redis; a key-value server. I designed this project in order to further my knowledge of in-memory databases and client – server interaction.

This project initially contained the PING, SET and GET functions. Functions were then progressively developed. Further functionality included that of key expiry and also multi-client interaction. Data can also be persisted to file.

## Overview

Mini Redis can hold and manipulate data in the form of key-value pairs.

For instance, the key ‘city’ could contain the value ‘Gwalior’. Clients can interact with the server and issue commands to add, retrieve, modify and remove data.

The server listens for commands on TCP port 6379. Commands are issued in a Redis inspired format known as RESP.

## Features

- Memory based key-value store.
- Multi-client interaction facilitated by threads.
- Mutex used to synchronise access to the database.
- Key expiry is enabled via a TTL.
- Supported functions are recorded within an AOF file.
- Data can be reloaded upon server restart.
- A bash script has been produced to automate command testing.

## Implemented Commands

### Basic
- PING
- SET
- GET
- DEL

### Key related
- EXISTS
- TYPE
- RENAME
- RENAMENX

### Expiry
- EXPIRE
- EXPIREAT
- TTL
- PERSIST

### Multi-key
- MSET
- MGET

### Numeric
- INCR
- DECR
- INCRBY
- DECRBY

### String
- STRLEN
- APPEND

### Other
- GETSET
- SETNX
- GETDEL

### Database related
- DBSIZE
- FLUSHALL

## Tools

- C++ – for the server and database.
- TCP sockets – for client – server interaction.
- Threads and mutex – to enable safe multi-client interaction.
- unordered_map – for the key-value store.
- File tools – for data file persistence.
- Bash script and Netcat – for command testing.
- VS Code and WSL Ubuntu – for development.

## Folder Structure

```text
mini-redis-cpp/
│
├── src/
│   └── server.cpp
│
├── include/
│
├── tests/
│   └── test_commands.sh
│
├── README.md
├── appendonly.aof
└── server
```

## How To Execute

This project was developed and tested on Ubuntu (via WSL).

### 1. Install relevant tools

```bash
sudo apt update
sudo apt install g++ netcat-openbsd
```

### 2. Compile the server

Launch a terminal in the project folder and TYPE:

```bash
g++ src/server.cpp -o server -pthread
```

### 3. Execute the server

```bash
./server
```

The server will now listen for commands on port 6379.

Leave this terminal active during command testing.

## Command Testing

Launch another terminal in the project folder.

Execute the test bash script:

```bash
./tests/test_commands.sh
```

This script issues commands to the executing server and validates the resulting output.

Currently there are 23 automated tests which are all succeeding.

## Some Command Examples

Netcat can be used to issue RESP formatted commands to the server.

### PING

```bash
printf '1\r\n$4\r\nPING\r\n' | nc -w 2 127.0.0.1 6379
```

**Output:**

```text
+PONG
```

### SET

```bash
printf '3\r\n$3\r\nSET\r\n$4\r\ncity\r\n$7\r\nGwalior\r\n' | nc -w 2 127.0.0.1 6379
```

**Output:**

```text
+OK
```

### GET

```bash
printf '*2\r\n$3\r\nGET\r\n$4\r\ncity\r\n' | nc -w 2 127.0.0.1 6379
```

**Output:**

```text
$7

Gwalior
```

## Data Persistence

An Append-Only File called ‘appendonly.aof’ is utilised to persist data from the database.

Each time a supported write function is executed, the relevant information is appended to this file. Upon server restart, the file is analysed and the data is reloaded.

This was useful in demonstrating how data can be re-established following a server restart.

## What I learnt

During the course of this project I was able to:

- Develop a TCP server using sockets.
- Investigate client – server interaction.
- Employ C++ threads and mutex.
- Leverage maps for memory data storage.
- Implement key expiry.
- Interact with files to read and write data.
- Construct bash scripts to automate testing.

## Limitations

Although this is an educational project, it is not a full alternative to Redis.

- Not all Redis commands have been developed.
- Primarily, only string data TYPE support is provided for.
- The RESP analysis process is basic and does not fully address all aspects of the protocol.
- More sophisticated Redis function and data TYPE support is not present.
- Neither AOF rewriting nor file optimisation is currently available.

## Areas For Further Development

There are further opportunities to enhance this project. Potential developments include:

- Enhance the RESP analysis.
- Develop additional Redis commands and data types.
- Refine input validation and error processing.
- Provide AOF rewriting.
- Develop further test scripts, particularly for extreme situations.

## Author

**Anmol Kumar Pastor**

B.Tech, Information Technology

MITS Gwalior
