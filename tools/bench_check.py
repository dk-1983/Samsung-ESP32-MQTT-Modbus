"""Check the real ESP32 Modbus TCP server on a bench. UART simulator must be running.
Default is read-only. --exercise writes a target temperature and restores it (simulator only).
"""
import argparse
import socket
import struct
import time

class Modbus:
    def __init__(self, host, unit):
        self.host, self.unit, self.transaction = host, unit, 0

    def request(self, fc, address, value):
        self.transaction = (self.transaction+1) & 65535
        packet = struct.pack('>HHHBBHH', self.transaction, 0, 6, self.unit, fc, address, value)
        with socket.create_connection((self.host, 502), timeout=3) as link:
            link.settimeout(3)
            link.sendall(packet)
            def exact(n):
                b = b''
                while len(b) < n:
                    part = link.recv(n-len(b))
                    if not part:
                        raise RuntimeError('ESP32 closed the connection')
                    b += part
                return b
            head = exact(7)
            transaction, protocol, size, unit = struct.unpack('>HHHB', head)
            if (transaction, protocol, unit) != (self.transaction, 0, self.unit) or not 2 <= size <= 254:
                raise RuntimeError('Invalid MBAP header')
            body = exact(size-1)
        if body[0] == fc | 0x80:
            return None, body[1]
        if body[0] != fc:
            raise RuntimeError('Wrong Modbus function in response')
        return body, 0

    def read(self, fc, address, count=1):
        body, error = self.request(fc, address, count)
        if error:
            raise RuntimeError(f'Read {fc:02X}/{address} rejected: exception {error:02X}')
        if len(body) != 2+count*2 or body[1] != count*2:
            raise RuntimeError('Invalid register response length')
        return list(struct.unpack('>'+'H'*count, body[2:]))

    def set_target(self, target):
        for _ in range(8):
            body, error = self.request(6, 58, target)
            if error == 6:
                time.sleep(0.6)
                continue
            if error or body != struct.pack('>BHH', 6, 58, target):
                raise RuntimeError(f'Target write rejected/invalid: {error:02X}')
            break
        else:
            raise RuntimeError('UART remains busy or TX is disabled')
        deadline = time.monotonic()+12
        while time.monotonic() < deadline:
            status = self.read(4, 2478)[0]
            if status == 2:
                if self.read(3, 58)[0] != target:
                    raise RuntimeError('Confirmed state does not match target')
                return
            if status in (3, 4):
                raise RuntimeError('Command timed out or was cancelled')
            time.sleep(0.25)
        raise RuntimeError('No confirmed feedback')

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--host', required=True)
    parser.add_argument('--unit', type=int, default=1)
    parser.add_argument('--exercise', action='store_true', help='SIMULATOR ONLY: change and restore target')
    args = parser.parse_args()
    if not 1 <= args.unit <= 247:
        parser.error('--unit must be 1..247')
    mb = Modbus(args.host, args.unit)
    power, mode, fan, swing = mb.read(3, 52, 4)
    target = mb.read(3, 58)[0]
    diagnostics = mb.read(4, 2475, 6)
    print(f'Target={target/10} C, mode={mode}, fan={fan}; diagnostics[2475..2480]={diagnostics}')
    _, absent = mb.request(3, 64, 1)
    if absent != 2:
        raise RuntimeError('Unsupported lock register must return exception 02')
    if not diagnostics[4] & 1:
        raise RuntimeError('Core feedback is not fresh; check simulator RX wiring')
    print('PASS: live feedback, diagnostic registers, absent register rejection')
    if args.exercise:
        if not diagnostics[5]:
            raise RuntimeError('Enable UART transmission in the local web UI first')
        changed = 230 if target == 240 else 240
        try:
            mb.set_target(changed)
            print(f'PASS: target {changed} confirmed via UART feedback')
        finally:
            # Best-effort restoration is itself checked; never hide its failure.
            time.sleep(0.6)
            mb.set_target(target)
            print(f'PASS: original target {target} restored and confirmed')

if __name__ == '__main__':
    main()
