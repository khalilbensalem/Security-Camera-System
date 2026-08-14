# Server — Odroid-C2

The server side of the security camera system. Runs on an Odroid-C2 board
under an embedded Linux (Yocto) distribution. Owns the USB camera, a GPIO
push-button, and a photoresistor read through the IIO subsystem, and
serves video and hardware events to a single connected client over
TCP/IP.

## Responsibilities

- Accepts one TCP client at a time on port 4099 and answers `GET_FRAME`
  requests with a JPEG-compressed image (800×600, ~30 fps target).
- Continuously captures from the USB camera on a **dedicated background
  thread** (`Camera::captureLoop`), so a slow or blocked read never stalls
  the network loop. `captureFrame()` just hands back the most recent frame
  under a mutex.
- Monitors a GPIO push-button on its own thread (`GpioButton::monitorLoop`,
  via `libgpiod`), with software debouncing. A press is reported as a
  `BUTTON_PRESS` response on the next frame request.
- Reads a photoresistor via `sysfs`/IIO (`/sys/bus/iio/devices/.../
  in_voltage0_raw`) and cross-checks it against measured image brightness
  to report `NO_LIGHT` (scene genuinely dark) or `SENSOR_ERROR` (camera and
  sensor disagree — e.g. lens covered while the sensor reads bright).
  State changes are debounced (200 ms) to avoid flicker.
- Detects a dead network link (no `FIN`/`RST` on a physically unplugged
  cable) via an inactivity timeout, closes the stale connection, and
  returns to accepting a new client — it never terminates on a network
  failure.
- Can be configured to start automatically at board boot via a SysVinit
  service (this Yocto image doesn't ship systemd).

## Protocol (server side)

| Code | Value | Meaning | Direction |
|---|---|---|---|
| `GET_FRAME` | 1 | Request a captured frame | client → server |
| `STOP` | 2 | Request graceful shutdown | client → server |
| `FRAME_HDR` | 101 | Frame header (id, size, JPEG follows) | server → client |
| `STOP_ACK` | 102 | Shutdown acknowledged | server → client |
| `BUTTON_PRESS` | 103 | Frame tied to a button press | server → client |
| `NO_LIGHT` | 201 | Scene too dark, no image sent | server → client |
| `SENSOR_ERROR` | 202 | Sensor/image mismatch, no image sent | server → client |

## Design notes worth knowing

- **Single network thread, strictly synchronous.** One connection is
  handled at a time: read a command, process it fully (including the
  whole frame pipeline), respond, loop. This keeps the protocol simple but
  means the server can't preempt a slow response to check for an incoming
  `STOP` — a deliberate tradeoff, compensated for on the client side.
- **The camera can physically disappear.** The Odroid-C2's USB camera
  re-enumerates during boot (visible in `dmesg` as a disconnect/reconnect
  pair). `Camera::openCamera()` scans `/dev/video0` through `/dev/video9`
  and validates each candidate with an actual test read — not just a
  successful `open()` — and the capture thread will attempt to reopen
  after a run of consecutive read failures.
- **A failed capture still gets a response.** Early on, a camera read
  failure meant no response at all to `GET_FRAME`, which the client
  (waiting on a fixed timeout) misread as a lost connection. The fix
  falls back to the light sensor alone to still classify the frame as
  `NO_LIGHT` or `SENSOR_ERROR`.
- **Draining the socket before closing after `STOP`.** Closing a TCP
  socket with unread bytes still sitting in the receive buffer makes the
  kernel send a `RST` instead of a clean `FIN` — which can break a
  subsequent `send()` on the client side. The server drains any pending
  bytes first.

## Build

```bash
mkdir build && cd build
cmake ..
make
```

Requires OpenCV and `libgpiod` (development headers). Cross-compiled and
deployed to the target via the toolchain/container set up for this course;
adjust `CMakeLists.txt` include/library paths for your own environment if
building outside that setup.

## Hardware assumptions

- USB camera (any UVC-compatible device works; resolution/format are
  negotiated in `Camera::openCamera()`).
- GPIO push-button on `gpiochip1`, line 92 (adjust in `TcpServer::init()`
  for a different wiring).
- Photoresistor wired to an ADC channel exposed via IIO
  (`in_voltage0_raw` on the board's `meson-gxbb-saradc` chip); thresholds
  in `LightSensor.cpp` are tuned empirically for that specific circuit and
  will need recalibration on different hardware.
