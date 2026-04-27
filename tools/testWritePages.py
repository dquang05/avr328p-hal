# This script is used to verify that the bootloader can receive and write pages correctly.

import argparse
import time
from dataclasses import dataclass

import serial


ACK = b"A"
NACK = b"N"

CMD_MAGIC = b"U"
CMD_PING = b"P"
CMD_WRITE = b"W"
CMD_JUMP = b"J"

PAGE_SIZE = 128
DEFAULT_BAUD = 9600


@dataclass
class BootSender:
    ser: serial.Serial

    def read_for(self, duration_s: float) -> bytes: 
        """Read all available bytes for a fixed duration."""
        end_time = time.time() + duration_s
        rx = bytearray()

        while time.time() < end_time:
            chunk = self.ser.read(64)
            # If no more bytes are available, chunk will be empty and we can stop early.
            if chunk: 
                rx.extend(chunk)

        return bytes(rx)

    def wait_for_ack(self, timeout_s: float = 1.0) -> bool:
        """Read bytes until ACK or NACK is found."""
        end_time = time.time() + timeout_s

        while time.time() < end_time:
            b = self.ser.read(1)

            if b == ACK:
                return True

            if b == NACK:
                return False

        return False

    def enter_update_mode(self, duration_s: float = 3.0) -> bool:
        """
        Try to enter bootloader command mode.

        This does not depend on debug text such as 'Enter Update Mode'.
        The reliable condition is PING -> ACK.
        """
        start_time = time.time()

        while time.time() - start_time < duration_s:
            self.ser.write(CMD_MAGIC)
            time.sleep(0.05)

            # Drain debug text or rubbish bytes.
            self.read_for(0.05)

            self.ser.write(CMD_PING)

            if self.wait_for_ack(timeout_s=0.2):
                return True

        return False

    def ping(self) -> bool:
        """Send ping command and wait for ACK."""
        self.ser.write(CMD_PING)
        return self.wait_for_ack(timeout_s=1.0)

    def build_write_frame(self, page_addr: int, data: bytes) -> bytes:
        """Build one W frame: W + addr[4] + len[2] + data[128] + checksum[1]."""
        if len(data) != PAGE_SIZE:
            raise ValueError("data must be exactly one flash page")

        if page_addr % PAGE_SIZE != 0:
            raise ValueError("page_addr must be page-aligned")

        header = (
            page_addr.to_bytes(4, "little") +
            PAGE_SIZE.to_bytes(2, "little")
        )

        checksum = (ord("W") + sum(header) + sum(data)) & 0xFF

        return CMD_WRITE + header + data + bytes([checksum])

    def write_page(self, page_addr: int, data: bytes) -> bool:
        """Write one page and wait for ACK."""
        frame = self.build_write_frame(page_addr, data)

        self.ser.reset_input_buffer()
        self.ser.write(frame)

        return self.wait_for_ack(timeout_s=2.0)

    def jump_to_app(self) -> bool:
        """Send jump command."""
        self.ser.write(CMD_JUMP)
        return self.wait_for_ack(timeout_s=1.0)


def make_test_page(seed: int) -> bytes:
    """Generate deterministic test data for one page."""
    return bytes(((i + seed) & 0xFF) for i in range(PAGE_SIZE))


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", required=True)
    parser.add_argument("--baud", type=int, default=DEFAULT_BAUD)
    parser.add_argument("--addr", type=lambda x: int(x, 0), default=0x7B80)
    parser.add_argument("--pages", type=int, default=1)
    parser.add_argument("--jump", action="store_true")
    args = parser.parse_args()

    with serial.Serial(args.port, args.baud, timeout=0.05) as ser:
        sender = BootSender(ser)

        time.sleep(0.2)
        ser.reset_input_buffer()
        ser.reset_output_buffer()

        print("Reset MCU now, then press Enter.")
        input()

        if not sender.enter_update_mode(duration_s=3.0):
            print("Failed to enter update mode")
            return

        print("Entered update mode")

        if not sender.ping():
            print("Ping failed")
            return

        print("Ping OK")

        for page_index in range(args.pages):
            page_addr = args.addr + page_index * PAGE_SIZE
            data = make_test_page(seed=page_index)

            ok = sender.write_page(page_addr, data)

            if not ok:
                print(f"Write failed at 0x{page_addr:04X}")
                return

            print(f"Write OK at 0x{page_addr:04X}")

        if args.jump:
            if sender.jump_to_app():
                print("Jump command ACK")
            else:
                print("Jump command failed")


if __name__ == "__main__":
    main()