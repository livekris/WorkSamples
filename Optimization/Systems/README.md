
# Systems and networks: LEFM

**LEFMI** (Low Edge Flow Meter Interface) is a long-running daemon that communicates with an external **LEFM device** over TCP/IP.  
It is designed to run in the background for years without restart, ensuring reliable data acquisition, recovery from network hiccups, and safe classification of potentially fragmented or suspicious messages.

The project is built around **three main components**:

- **LEFMI**  
  Top-level application class. Manages the lifecycle of the daemon:
  - Opens/closes TCP connection (`LEFMData`)
  - Reads incoming data
  - Classifies/parses messages (`BufferData`)
  - Provides recovery logic for suspicious or corrupted transmissions

- **LEFMData**  
  Handles socket communication with the LEFM device:
  - `nOpenConnectionToLEFM()` – establishes a TCP connection
  - `nReadFromLEFM()` – reads data with `select()` and buffering
  - `nCloseConnectionToLEFM()` – gracefully closes the socket

- **BufferData**  
  Manages incoming message buffers and parsing:
  - Identifies start/end markers (`\x01` = SOH, `\x03` = EOT)
  - Distinguishes **GOOD**, **INCOMPLETE**, **SUSPICIOUS**, or **CORRUPT** data
  - Provides getters to retrieve fully reconstructed messages
  
## Build Instructions

### Prerequisites
- C++17 or newer
- POSIX sockets (Linux / Unix environment)
- Standard Make / g++ toolchain

### Compile
```
g++ -std=c++17 -Wall -O2 -o lefmi \
    BufferData.cpp LEFMData.cpp LEFMI.cpp main.cpp
  
./lefmi
```  
