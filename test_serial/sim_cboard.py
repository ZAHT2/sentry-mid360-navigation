#!/usr/bin/env python3
"""
Simulate C board on /tmp/ttyV1:
  - Sends UplinkFrame at 50 Hz (vx/vy/yaw/pos_x/pos_y slowly incrementing)
  - Reads and prints any DownlinkFrame received from MiniPC
"""

import serial
import struct
import time
import sys

FRAME_SOF     = 0xA5
CMD_UPLINK    = 0x01
CMD_DOWNLINK  = 0x02
UPLINK_LEN    = 24   # bytes
DOWNLINK_LEN  = 16   # bytes


def crc16(data: bytes) -> int:
    crc = 0xFFFF
    for b in data:
        crc ^= b
        for _ in range(8):
            crc = (crc >> 1) ^ 0xA001 if (crc & 0x0001) else (crc >> 1)
    return crc & 0xFFFF


def build_uplink(vx: float, vy: float, yaw: float,
                 pos_x: float, pos_y: float) -> bytes:
    payload = struct.pack('<BBfffff', FRAME_SOF, CMD_UPLINK,
                         vx, vy, yaw, pos_x, pos_y)
    crc = crc16(payload)
    return payload + struct.pack('<H', crc)


def parse_downlink(buf: bytearray):
    """Return (vx, vy, wz) if a valid DownlinkFrame is found, else None."""
    while len(buf) >= DOWNLINK_LEN:
        if buf[0] != FRAME_SOF or buf[1] != CMD_DOWNLINK:
            del buf[0]
            continue
        frame = bytes(buf[:DOWNLINK_LEN])
        computed = crc16(frame[:-2])
        received = struct.unpack_from('<H', frame, DOWNLINK_LEN - 2)[0]
        if computed != received:
            del buf[0]
            continue
        vx, vy, wz = struct.unpack_from('<fff', frame, 2)
        del buf[:DOWNLINK_LEN]
        return vx, vy, wz
    return None


def main():
    port = sys.argv[1] if len(sys.argv) > 1 else '/tmp/ttyV1'
    ser = serial.Serial(port, baudrate=115200, timeout=0)
    print(f'[sim_cboard] opened {port}')

    rx_buf = bytearray()
    t0 = time.time()
    interval = 1.0 / 50.0  # 50 Hz

    while True:
        t = time.time() - t0
        # Slowly move forward (+x) and rotate
        pos_x = 0.1 * t
        pos_y = 0.0
        yaw   = 0.05 * t
        vx    = 0.1
        vy    = 0.0

        frame = build_uplink(vx, vy, yaw, pos_x, pos_y)
        ser.write(frame)

        # Read any downlink data
        data = ser.read(64)
        if data:
            rx_buf.extend(data)
            result = parse_downlink(rx_buf)
            if result:
                print(f'[downlink] vx={result[0]:.3f}  vy={result[1]:.3f}  wz={result[2]:.3f}')

        time.sleep(interval)


if __name__ == '__main__':
    main()
