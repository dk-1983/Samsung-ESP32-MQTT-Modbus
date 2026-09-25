"""Bench-only Samsung D0 UART simulator. Connect to ESP32 via a 3.3 V USB-UART adapter.
Do not connect this simulator to an air conditioner. Requires pyserial (ESPHome dependency).
"""
import argparse
import functools
import time
import serial

def packet(kind, count, payload):
    head = bytes([0xD0, 0xC0, 2, 12+len(payload), 0, 0, 0, 0, 0, count, 0xFE,
                  kind >> 8, kind & 255, len(payload)]) + payload
    return head + bytes([functools.reduce(int.__xor__, head), 0xE0])

def main():
    args = argparse.ArgumentParser(description=__doc__)
    args.add_argument('--port', required=True, help='USB-UART connected ONLY to the ESP32 test UART')
    options = args.parse_args()
    state = {2: 0xF0, 0x43: 0x12, 0x5A: 24, 0x62: 0, 0x63: 0x12, 0x44: 0x12, 0x5C: 25}
    buf = bytearray()
    counter, last = 0, 0
    with serial.Serial(options.port, 9600, timeout=0.05) as link:
        def send(kind, count, payload):
            b = packet(kind, count, payload)
            link.write(b)
            print('SIM TX', b.hex(' '), flush=True)
        def snapshot():
            return bytes(x for k, v in state.items() for x in (k, 1, v))
        while True:
            buf.extend(link.read(256))
            while len(buf) >= 4:
                if buf[:3] != b'\xd0\xc0\x02':
                    del buf[0]
                    continue
                n = buf[3]+4
                if n < 16:
                    del buf[0]
                    continue
                if len(buf) < n:
                    break
                b = bytes(buf[:n])
                del buf[:n]
                if b[-1] != 0xE0 or b[13]+16 != n or functools.reduce(int.__xor__, b[:-2]) != b[-2]:
                    continue
                print('SIM RX', b.hex(' '), flush=True)
                kind = int.from_bytes(b[11:13], 'big')
                if kind == 0x1202:
                    send(0x1203, b[9], snapshot())
                elif kind == 0x1204:
                    i = 14
                    updates = {}
                    while i+2 <= n-2:
                        size = b[i+1]
                        if i+2+size > n-2:
                            break
                        if b[i] in state and size == 1:
                            updates[b[i]] = b[i+2]
                        i += size+2
                    if i != n-2:
                        continue
                    state.update(updates)
                    send(0x1205, b[9], b[14:-2])
                    time.sleep(0.1)
                    send(0x1206, b[9], snapshot())
            if time.monotonic()-last >= 3:
                send(0x1206, counter, snapshot())
                counter = (counter+1) & 255
                last = time.monotonic()

if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        pass
