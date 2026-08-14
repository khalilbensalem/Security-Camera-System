# Client — Workstation

The client side of the security camera system. Connects to the Odroid-C2
server, displays the live video feed in an OpenCV window, saves images on
button-press events, and — the most substantial part of this component —
detects and recovers from real network failures without leaving the video
stream in a corrupted state.

## Responsibilities

- Connects to the server over TCP/IP (port 4099) and requests a frame
  every 30 ms.
- Decodes and displays JPEG frames in an OpenCV window, overlaying the
  frame number.
- Saves images to a local `images/` directory when the server reports a
  `BUTTON_PRESS` event (directory is created if missing, purged on
  startup).
- Displays a warning screen with elapsed time (`Lumière insuffisante
  (t = 2.3 s)`) when the server reports `NO_LIGHT` or `SENSOR_ERROR`.
- **Detects a lost connection and reconnects automatically**, retrying
  every 500 ms with a non-blocking connection attempt bounded to 400 ms,
  so the UI never freezes waiting on the OS's default TCP connect timeout.
- Sends `STOP` on quitting (`q` key) and retries the handshake, since the
  server — strictly synchronous — may need noticeably longer than a single
  frame cycle to get around to reading it.
- Logs cycle timing stats (date/time, cycles/s, average cycle duration)
  to the terminal at least once per second.

## The interesting part: recovering from a desynchronized stream

TCP guarantees byte order, not message framing. This protocol has no
request identifier, so a client that abandons a partial read midway
through a response — say, a `recv()` timeout while still reading JPEG
bytes — has no way to know where the *next* message actually starts. The
leftover bytes are still in flight and will land in the buffer regardless.

An earlier version tolerated a few consecutive read failures (300 ms)
before declaring the connection lost, trying to ride out normal network
jitter on the same socket. That tolerance window turned out to be exactly
what caused stream corruption: the next `GET_FRAME` cycle would read those
orphaned bytes as if they were a fresh message header, misinterpreting
image data as a frame size — a runaway allocation waiting to happen, and a
cascade of failures until the socket finally got torn down anyway.

The fix: **fail fast**. Any single failed read is treated as connection
loss, immediately followed by tearing down the socket and reconnecting
from scratch. Without a request ID to resynchronize on, a fresh connection
is the only reliable way to guarantee a clean byte stream — trying to
"recover" on the same socket only prolongs the corruption.

## Reconnection mechanics

`TcpClient::tryReconnect()`:

1. Closes any existing socket (a socket that failed a `connect()` or was
   already closed can't be reused — a fresh `socket()` call is required).
2. Sets the new socket to non-blocking mode before calling `connect()`,
   since a blocking connect on a dead link can take several seconds to
   time out — far too slow for a 500 ms retry cadence.
3. Uses `select()` to wait for the connection to resolve (success or
   failure), bounded to 400 ms.
4. Checks `SO_ERROR` explicitly — `select()` only reports the socket is
   ready, not that the connection actually succeeded.
5. On success, restores blocking mode and the standard receive timeout.

## Build

```bash
mkdir build && cd build
cmake ..
make
```

Requires OpenCV. Adjust `SERVER_IP` in `main.cpp` to match your Odroid-C2's
address on your network.

## Known limitations

- The server's IP address is hardcoded (`main.cpp`); no discovery
  mechanism.
- `Ctrl+C` terminates the client without sending `STOP` — only the `q` key
  triggers the graceful shutdown handshake.
- No request identifier in the protocol means the "fail fast on any read
  error" strategy is a deliberate tradeoff: it prioritizes stream
  integrity over tolerating minor network jitter, since there's no safe
  way to distinguish the two without protocol changes.
