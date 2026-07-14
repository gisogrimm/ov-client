# Network Communication Protocol Documentation

## Overview
This protocol is used to transmit real-time multichannel audio over UDP/IP. It is designed for low-latency audio transmission between two JACK audio servers (Sender and Receiver). The protocol handles synchronization, sample rate conversion, and error recovery to ensure continuous audio playback.

## Architecture
The system consists of two main components:
1.  **Sender (`zita-j2n`)**: Captures audio from JACK, packetizes it, and sends it via UDP.
2.  **Receiver (`zita-n2j`)**: Receives UDP packets, buffers audio, performs synchronization and sample rate conversion, and outputs to JACK.

Communication is unidirectional (Sender -> Receiver) regarding audio data, though the Receiver manages synchronization internally based on packet timing.

## Transport Layer
*   **Protocol**: UDP
*   **Addressing**: Supports IPv4 and IPv6.
*   **Multicast**: Supports multicast transmission. If multicast is used, the sender sends to a multicast group, and the receiver joins that group on a specific network interface.

## Packet Structure
All packets share a common header. The protocol defines two primary packet types: **Audio Descriptor** and **Audio Data**.

### Byte Order
All multi-byte integer fields in the header are transmitted in **Network Byte Order** (Big Endian).

### Common Header
| Offset | Size | Type  | Description                              |
| :----- | :--- | :---- | :--------------------------------------- |
| 0      | 4    | char  | Magic Bytes: `'z'`, `'n'`, `'j'`, `'b'`  |
| 4      | 1    | uint8 | Packet Type (`PTYPE`)                    |
| 5      | 1    | uint8 | Flags (`FLAGS`)                          |
| 6      | 1    | uint8 | Sample Format (`SFORM`)                  |
| 7      | 1    | uint8 | Number of Channels (`NCHAN`)             |

### Packet Types (`PTYPE`)

#### 1. Audio Descriptor (`TY_ADESC`)
This packet is sent periodically by the transmitter to announce stream parameters. It is typically the first packet sent or sent repeatedly to keep the receiver informed.

**Header Extensions (Offset 8+):**
| Offset | Size | Type    | Description                                    |
| :----- | :--- | :------ | :--------------------------------------------- |
| 8      | 4    | int32   | Maximum Packet Size (`PSMAX`)                  |
| 12     | 4    | int32   | Sample Rate (`FSAMP`)                          |
| 16     | 4    | int32   | Period Size (frames per JACK period) (`FSIZE`) |
| 20     | 4    | int32   | Timestamp Frame Count (`TFCNT`)                |
| 24     | 4    | uint32  | Timestamp Seconds (`TSECS`) - NTP format       |
| 28     | 4    | uint32  | Timestamp Fraction (`TFRAC`) - NTP format      |

**Total Header Size:** 32 bytes.

#### 2. Audio Data (`TY_ADATA`)
This packet carries the actual audio samples.

**Header Extensions (Offset 8+):**
| Offset | Size | Type  | Description                                                      |
| :----- | :--- | :---- | :--------------------------------------------------------------- |
| 8      | 4    | int32 | Frame Count (`COUNT`) - Monotonically increasing sequence number |
| 12     | 4    | int32 | Number of Frames in this packet (`NFRAM`)                        |
| 16     | 4    | int32 | Delta Time (`DTIME`) - Microseconds since start of JACK period   |
| 20     | ...  | ...   | **Audio Payload**                                               |

**Audio Payload (Offset 20):**
Audio samples are interleaved. The size depends on `SFORM`, `NCHAN`, and `NFRAM`.
*   **Format**: Interleaved samples [Channel 0, Channel 1, ... Channel N, Channel 0, ...].
*   **Formats**:
    *   `FM_16BIT`: 16-bit signed integer (2 bytes/sample).
    *   `FM_24BIT`: 24-bit signed integer (3 bytes/sample, stored in 3 bytes).
    *   `FM_FLOAT`: 32-bit float (4 bytes/sample).

## Flags (`FLAGS`)
Bitmask used in the header.
*   `0x01` (`FL_TIMED`): Indicates the `DTIME` field is valid. This is set on the first packet of every JACK period.
*   `0x02` (`FL_SUSP`): Transmission suspended (e.g., JACK freewheeling).
*   `0x04` (`FL_SKIP`): Placeholder for skipped frames (not typically used in standard flow).
*   `0x80` (`FL_TERM`): Sender is terminating.

## Sample Formats (`SFORM`)
*   `0`: `FM_16BIT` (16-bit PCM)
*   `1`: `FM_24BIT` (24-bit PCM)
*   `2`: `FM_FLOAT` (32-bit Float)

## Protocol Operation

### 1. Initialization
*   The Receiver starts listening on the configured IP/Port.
*   The Sender starts sending `TY_ADESC` packets (and subsequently `TY_ADATA`).
*   The Receiver waits for a valid `TY_ADESC` packet to configure its internal buffers, sample rate converter, and JACK ports based on the received `FSAMP`, `FSIZE`, and `NCHAN`.

### 2. Data Transmission
*   The Sender reads audio from JACK.
*   It splits the JACK period into multiple UDP packets to fit within the Path MTU (default 1500 bytes).
*   The first packet of each period has the `FL_TIMED` flag set. The `DTIME` field contains the time elapsed since the start of the JACK cycle. This allows the Receiver to calculate network jitter and latency independent of the Sender's JACK graph position.
*   The `COUNT` field increments by the number of frames sent in each packet, allowing the Receiver to detect lost packets.

### 3. Reception & Synchronization
*   The Receiver reads packets from the socket.
*   It checks the `COUNT` field. If packets are missing (gap in `COUNT`), the Receiver inserts silence (zeros) into the audio buffer to maintain timing continuity.
*   The Receiver uses a DLL (Delay Locked Loop) algorithm. It compares the expected arrival time (based on `COUNT` and sample rate) with the actual arrival time (adjusted by `DTIME`).
*   This error drives a sample rate converter (`VResampler`) at the output stage, slightly speeding up or slowing down playback to match the Sender's clock and drain the buffer at the correct rate.

### 4. Termination
*   If the Sender stops gracefully, it sends a packet with `FL_TERM` flag set.
*   If the Receiver detects a timeout or fatal error, it stops playback and resets.

## Data Structures

### `Timedata` (Internal Queue)
Used internally to pass timing information from the network thread to the JACK thread.
```cpp
struct Timedata {
    int32_t  _flags; // Netrx::PROC, Netrx::WAIT, etc.
    int32_t  _count; // Frame count
    double   _tjack; // Jack time (modulo 2^28 us)
    uint32_t _tsecs; // NTP seconds
    uint32_t _tfrac; // NTP fraction
};
```

### `Infodata` (Internal Queue)
Used internally to pass status information (sync state, errors) to the main application.
```cpp
struct Infodata {
    int32_t  _state; // Jackrx state (SYNC0, SYNC1, PROC1, etc.)
    double   _error; // Loop filter error value
    double   _ratio; // Resampling ratio
    int      _nfram; // Number of frames in queue
    int      _syncc; // Sync counter
};
```
