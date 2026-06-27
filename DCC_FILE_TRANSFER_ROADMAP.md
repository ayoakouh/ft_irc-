# ft_irc — DCC File Transfer Implementation Roadmap

> This document is a step-by-step guide covering everything you need to add, change,
> or edit in your `ft_irc` server to support IRC file transfer via the **DCC (Direct
> Client-to-Client)** protocol.

---

## Table of Contents

1. [How DCC Works](#1-how-dcc-works)
2. [What Your Server Does and Does NOT Do](#2-what-your-server-does-and-does-not-do)
3. [Project Structure Changes](#3-project-structure-changes)
4. [Step-by-Step Implementation](#4-step-by-step-implementation)
   - [Step 1 — CTCP / DCC Detection](#step-1--ctcp--dcc-detection)
   - [Step 2 — DCC Message Parser](#step-2--dcc-message-parser)
   - [Step 3 — Input Validation](#step-3--input-validation)
   - [Step 4 — Update `privmsg()`](#step-4--update-privmsg)
   - [Step 5 — DCC ACCEPT Relay (optional but recommended)](#step-5--dcc-accept-relay-optional-but-recommended)
   - [Step 6 — Logging](#step-6--logging)
5. [Full Code to Add](#5-full-code-to-add)
6. [Files to Create / Edit](#6-files-to-create--edit)
7. [Testing](#7-testing)
8. [Common Errors and Fixes](#8-common-errors-and-fixes)
9. [Bonus — DCC RESUME Support](#9-bonus--dcc-resume-support)
10. [Checklist](#10-checklist)

---

## 1. How DCC Works

DCC (Direct Client-to-Client) is a sub-protocol of IRC used to transfer files directly
between two clients — **bypassing the server entirely** for the actual file bytes.

The flow is:

```
Sender (Client A)              Your IRC Server              Receiver (Client B)
       |                              |                              |
       |-- PRIVMSG nick :\x01DCC --> |                              |
       |   SEND file.txt ip port sz\x01                             |
       |                              |--- relay same PRIVMSG ----> |
       |                              |                              |
       |                    Server job ends here                     |
       |                                                             |
       |<============ Direct TCP connection on <ip>:<port> ========>|
       |              (file bytes flow here, server not involved)    |
```

The CTCP message your server must relay looks like this:

```
PRIVMSG <target_nick> :\x01DCC SEND <filename> <ip_as_long_int> <port> <filesize>\x01
```

- `\x01` is the ASCII character 0x01 (CTCP delimiter)
- `<ip_as_long_int>` is the sender's IP encoded as a 32-bit unsigned integer
- `<port>` is the port the sender is listening on
- `<filesize>` is the file size in bytes

The receiver then connects directly to `<ip>:<port>` to download the file.

**Your server's only job is to relay this PRIVMSG without corrupting it.**

---

## 2. What Your Server Does and Does NOT Do

| Responsibility                         | Who handles it   |
|----------------------------------------|------------------|
| Relay the DCC SEND CTCP message        | ✅ Your server    |
| Relay the DCC ACCEPT CTCP message      | ✅ Your server    |
| Open a listening socket for the file   | ❌ Sender client  |
| Connect and download the file          | ❌ Receiver client|
| Transfer file bytes                    | ❌ Never the server|
| Store files                            | ❌ Never the server|

Your server **never touches the file data**. It only passes the handshake message
between the two clients, exactly like it does for normal PRIVMSG.

---

## 3. Project Structure Changes

You need to add the following files and edit one existing file:

```
ft_irc/
├── srcs/
│   ├── commands/
│   │   ├── privmsg.cpp          ← EDIT THIS FILE
│   │   └── ...
│   ├── dcc/
│   │   ├── Dcc.hpp              ← CREATE THIS FILE
│   │   └── Dcc.cpp              ← CREATE THIS FILE
│   └── ...
├── includes/
│   └── ...
└── Makefile                     ← ADD dcc/Dcc.cpp to sources
```

---

## 4. Step-by-Step Implementation

---

### Step 1 — CTCP / DCC Detection

**File:** `srcs/dcc/Dcc.hpp`

A CTCP message starts and ends with the `\x01` byte (ASCII 0x01).
A DCC SEND message contains the literal text `DCC SEND` inside those delimiters.

```cpp
// Returns true if the message is a CTCP DCC SEND request
bool isDccSend(const std::string &msg);

// Returns true if the message is a CTCP DCC ACCEPT request
bool isDccAccept(const std::string &msg);

// Returns true if the message is ANY CTCP message (starts/ends with \x01)
bool isCtcp(const std::string &msg);
```

**File:** `srcs/dcc/Dcc.cpp`

```cpp
bool isCtcp(const std::string &msg)
{
    return (msg.size() >= 2 && msg[0] == '\x01' && msg[msg.size() - 1] == '\x01');
}

bool isDccSend(const std::string &msg)
{
    return isCtcp(msg) && msg.find("DCC SEND") != std::string::npos;
}

bool isDccAccept(const std::string &msg)
{
    return isCtcp(msg) && msg.find("DCC ACCEPT") != std::string::npos;
}
```

---

### Step 2 — DCC Message Parser

**File:** `srcs/dcc/Dcc.hpp`

Add a struct to hold parsed DCC SEND info:

```cpp
struct DccSendInfo
{
    std::string filename;
    std::string ip;       // raw long-integer string as sent by client
    std::string port;
    std::string filesize;
    bool        valid;

    DccSendInfo() : valid(false) {}
};

DccSendInfo parseDccSend(const std::string &msg);
```

**File:** `srcs/dcc/Dcc.cpp`

```cpp
// Splits a string by whitespace into a vector of tokens
static std::vector<std::string> splitBySpace(const std::string &s)
{
    std::vector<std::string> tokens;
    std::string              token;
    for (size_t i = 0; i < s.size(); i++)
    {
        if (s[i] == ' ')
        {
            if (!token.empty())
                tokens.push_back(token);
            token = "";
        }
        else
            token += s[i];
    }
    if (!token.empty())
        tokens.push_back(token);
    return tokens;
}

DccSendInfo parseDccSend(const std::string &msg)
{
    DccSendInfo info;

    // Strip the leading and trailing \x01
    if (msg.size() < 2)
        return info;
    std::string inner = msg.substr(1, msg.size() - 2);

    // Expected tokens: DCC SEND <filename> <ip> <port> <filesize>
    // Index:            0   1      2          3    4       5
    std::vector<std::string> tokens = splitBySpace(inner);

    if (tokens.size() < 6)
        return info;
    if (tokens[0] != "DCC" || tokens[1] != "SEND")
        return info;

    info.filename = tokens[2];
    info.ip       = tokens[3];
    info.port     = tokens[4];
    info.filesize = tokens[5];
    info.valid    = true;

    return info;
}
```

> **Note on filenames with spaces:** Some IRC clients quote filenames that contain
> spaces (e.g. `"my file.txt"`). The parser above treats the quoted name as a single
> token only if the client sends it without the space. For maximum compatibility you
> can add quote-stripping logic, but most modern clients avoid spaces in DCC filenames.

---

### Step 3 — Input Validation

**File:** `srcs/dcc/Dcc.hpp`

```cpp
bool isValidDccPort(const std::string &port);
bool isValidDccFilesize(const std::string &filesize);
bool isValidDccFilename(const std::string &filename);
```

**File:** `srcs/dcc/Dcc.cpp`

```cpp
// Port must be numeric and in the range 1025–65535
bool isValidDccPort(const std::string &port)
{
    if (port.empty())
        return false;
    for (size_t i = 0; i < port.size(); i++)
        if (!std::isdigit(port[i]))
            return false;
    long p = std::atol(port.c_str());
    return (p > 1024 && p <= 65535);
}

// Filesize must be a non-empty string of digits (no negatives)
bool isValidDccFilesize(const std::string &filesize)
{
    if (filesize.empty())
        return false;
    for (size_t i = 0; i < filesize.size(); i++)
        if (!std::isdigit(filesize[i]))
            return false;
    return true;
}

// Filename must not be empty and must not contain path separators
bool isValidDccFilename(const std::string &filename)
{
    if (filename.empty())
        return false;
    if (filename.find('/') != std::string::npos)
        return false;
    if (filename.find('\\') != std::string::npos)
        return false;
    return true;
}
```

---

### Step 4 — Update `privmsg()`

**File:** `srcs/commands/privmsg.cpp`

Add the include at the top of the file:

```cpp
#include "dcc/Dcc.hpp"
```

Inside the `else` block that handles **private messages to a single user**, replace
your current `send_M` construction and send with the following:

```cpp
// --- BEFORE (your current code) ---
std::string send_M = ":" + clients_map[fd].getNickname()
          + "!" + clients_map[fd].getUsername()
          + "@ft_irc PRIVMSG " + target + " :" + s[2] + "\r\n";
send(tclient->getFd(), send_M.c_str(), send_M.size(), 0);


// --- AFTER (replace with this block) ---
std::string message = s[2];

if (isDccSend(message))
{
    // Parse and validate the DCC SEND request
    DccSendInfo dcc = parseDccSend(message);

    if (!dcc.valid)
    {
        std::string err = ":ft_irc 400 " + clients_map[fd].getNickname()
                        + " :Malformed DCC SEND request\r\n";
        send(fd, err.c_str(), err.size(), 0);
        continue;
    }
    if (!isValidDccFilename(dcc.filename))
    {
        std::string err = ":ft_irc 400 " + clients_map[fd].getNickname()
                        + " :Invalid DCC filename\r\n";
        send(fd, err.c_str(), err.size(), 0);
        continue;
    }
    if (!isValidDccPort(dcc.port))
    {
        std::string err = ":ft_irc 400 " + clients_map[fd].getNickname()
                        + " :Invalid DCC port (must be 1025-65535)\r\n";
        send(fd, err.c_str(), err.size(), 0);
        continue;
    }
    if (!isValidDccFilesize(dcc.filesize))
    {
        std::string err = ":ft_irc 400 " + clients_map[fd].getNickname()
                        + " :Invalid DCC filesize\r\n";
        send(fd, err.c_str(), err.size(), 0);
        continue;
    }

    // Log the DCC request
    std::cout << "[DCC SEND] "
              << clients_map[fd].getNickname()
              << " -> " << target
              << " | file=" << dcc.filename
              << " | size=" << dcc.filesize << " bytes"
              << " | port=" << dcc.port
              << std::endl;
}
else if (isDccAccept(message))
{
    // DCC ACCEPT is used for resuming interrupted transfers
    // The server just relays it — no extra validation required
    std::cout << "[DCC ACCEPT] "
              << clients_map[fd].getNickname()
              << " -> " << target << std::endl;
}

// Relay the message (DCC handshake or plain text) to the target client
std::string send_M = ":" + clients_map[fd].getNickname()
          + "!" + clients_map[fd].getUsername()
          + "@ft_irc PRIVMSG " + target + " :" + message + "\r\n";
send(tclient->getFd(), send_M.c_str(), send_M.size(), 0);
```

---

### Step 5 — DCC ACCEPT Relay (optional but recommended)

DCC ACCEPT is used when the receiver wants to **resume** a partially received file.
Its format is:

```
PRIVMSG <nick> :\x01DCC ACCEPT <filename> <port> <resume_position>\x01
```

Your server just needs to relay it like any other CTCP message. The code in Step 4
already handles this via the `isDccAccept` branch — no extra routing logic is needed.

---

### Step 6 — Logging

Add structured log output so you can verify DCC activity during evaluation.
The log lines added in Step 4 already cover the basics. You can centralise them
by adding a small helper:

**File:** `srcs/dcc/Dcc.hpp`

```cpp
void logDcc(const std::string &type,
            const std::string &sender,
            const std::string &receiver,
            const DccSendInfo &info);
```

**File:** `srcs/dcc/Dcc.cpp`

```cpp
void logDcc(const std::string &type,
            const std::string &sender,
            const std::string &receiver,
            const DccSendInfo &info)
{
    std::cout << "[" << type << "] "
              << sender << " -> " << receiver;
    if (!info.filename.empty())
        std::cout << " | file=" << info.filename
                  << " | size=" << info.filesize
                  << " | port=" << info.port;
    std::cout << std::endl;
}
```

---

## 5. Full Code to Add

### `srcs/dcc/Dcc.hpp`

```cpp
#ifndef DCC_HPP
# define DCC_HPP

# include <string>
# include <vector>
# include <iostream>
# include <cstdlib>
# include <cctype>

struct DccSendInfo
{
    std::string filename;
    std::string ip;
    std::string port;
    std::string filesize;
    bool        valid;

    DccSendInfo() : valid(false) {}
};

bool        isCtcp(const std::string &msg);
bool        isDccSend(const std::string &msg);
bool        isDccAccept(const std::string &msg);

DccSendInfo parseDccSend(const std::string &msg);

bool        isValidDccPort(const std::string &port);
bool        isValidDccFilesize(const std::string &filesize);
bool        isValidDccFilename(const std::string &filename);

void        logDcc(const std::string &type,
                   const std::string &sender,
                   const std::string &receiver,
                   const DccSendInfo &info);

#endif
```

### `srcs/dcc/Dcc.cpp`

Combine all function bodies from Steps 1–3 and 6 above into this single file.

---

## 6. Files to Create / Edit

| File                        | Action   | What changes                                          |
|-----------------------------|----------|-------------------------------------------------------|
| `srcs/dcc/Dcc.hpp`          | CREATE   | Struct + function declarations for all DCC helpers    |
| `srcs/dcc/Dcc.cpp`          | CREATE   | Implementations of all DCC helpers                    |
| `srcs/commands/privmsg.cpp` | EDIT     | Add DCC detection + validation block in the else branch|
| `Makefile`                  | EDIT     | Add `srcs/dcc/Dcc.cpp` to your `SRCS` variable        |

### Makefile change

Find your `SRCS` variable and add the new file:

```makefile
SRCS =  srcs/main.cpp \
        srcs/Server.cpp \
        srcs/Client.cpp \
        srcs/Channel.cpp \
        srcs/commands/privmsg.cpp \
        srcs/dcc/Dcc.cpp          # ← ADD THIS LINE
```

---

## 7. Testing

### Test 1 — Basic DCC SEND relay with `nc`

```bash
# Terminal 1: start your server
./ircserv 6667 testpass

# Terminal 2: simulate sender
nc -C 127.0.0.1 6667
PASS testpass
NICK sender
USER sender 0 * :Sender

# Terminal 3: simulate receiver
nc -C 127.0.0.1 6667
PASS testpass
NICK receiver
USER receiver 0 * :Receiver

# Back in Terminal 2: send a DCC SEND CTCP
PRIVMSG receiver :^ADCC SEND testfile.txt 2130706433 5000 12345^A
# (^A is the literal \x01 character — type Ctrl+V then Ctrl+A in most terminals)
```

Expected: Terminal 3 receives:
```
:sender!sender@ft_irc PRIVMSG receiver :\x01DCC SEND testfile.txt 2130706433 5000 12345\x01
```

### Test 2 — Invalid port rejection

Send a DCC SEND with port `80` (below 1025). Your server should reply:
```
:ft_irc 400 sender :Invalid DCC port (must be 1025-65535)
```

### Test 3 — Real client DCC

Use an IRC client that supports DCC (e.g. HexChat or irssi):

1. Connect two instances to your server on port 6667.
2. From one client, right-click the other user and choose **DCC Send**.
3. Select a small test file.
4. The receiving client should get a DCC offer popup and be able to accept.

---

## 8. Common Errors and Fixes

| Symptom                                      | Cause                                              | Fix                                                        |
|----------------------------------------------|----------------------------------------------------|------------------------------------------------------------|
| DCC offer never arrives at receiver          | `\x01` bytes stripped by your server               | Make sure you forward `s[2]` verbatim, not a copy with special char stripped |
| `parseDccSend` returns `valid = false`        | Message has fewer than 6 tokens                    | Print `inner` to debug; client may omit filesize           |
| Receiver gets offer but connection fails      | NAT / firewall on sender's machine                 | Not your server's problem — it's a client network issue    |
| Crash on large filenames                      | `tokens[2]` out-of-range                           | Always check `tokens.size() >= 6` before indexing          |
| DCC works on Linux but not macOS             | `fcntl` flags missing on sender socket             | Unrelated to your server; client must handle this          |

---

## 9. Bonus — DCC RESUME Support

If you want to support resuming interrupted transfers (bonus points), your server
must also relay the **DCC RESUME** message:

```
PRIVMSG <nick> :\x01DCC RESUME <filename> <port> <position>\x01
```

Add to `Dcc.hpp` / `Dcc.cpp`:

```cpp
bool isDccResume(const std::string &msg)
{
    return isCtcp(msg) && msg.find("DCC RESUME") != std::string::npos;
}
```

Then add an `else if (isDccResume(message))` branch in `privmsg.cpp` — relay it
the same way as DCC ACCEPT, with a log line.

The full DCC resume exchange looks like:

```
Receiver  --> Server  --> Sender  : DCC RESUME file.txt 5000 4096
Sender    --> Server  --> Receiver: DCC ACCEPT file.txt 5000 4096
(direct connection resumes at byte offset 4096)
```

---

## 10. Checklist

Work through this list before your evaluation:

- [ ] `srcs/dcc/Dcc.hpp` created with `DccSendInfo` struct and all declarations
- [ ] `srcs/dcc/Dcc.cpp` created with all function bodies
- [ ] `Makefile` updated with `srcs/dcc/Dcc.cpp`
- [ ] `privmsg.cpp` includes `dcc/Dcc.hpp`
- [ ] DCC SEND detection added in the private-message branch of `privmsg()`
- [ ] Filename validation rejects paths with `/` or `\`
- [ ] Port validation rejects privileged ports (≤ 1024)
- [ ] Filesize validation rejects non-numeric values
- [ ] CTCP message relayed **verbatim** (no trimming of `\x01`)
- [ ] DCC ACCEPT relayed without extra validation
- [ ] Log output printed for every DCC request
- [ ] Tested with `nc` (manual CTCP injection)
- [ ] Tested with a real IRC client (HexChat or irssi)
- [ ] *(Bonus)* DCC RESUME and DCC ACCEPT relay implemented
