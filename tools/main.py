import argparse
import time
from dataclasses import dataclass
from pathlib import Path

import serial


ACK = b"A"
NACK = b"N"

CMD_MAGIC = b"U"
CMD_PING = b"P"
CMD_WRITE = b"W"
CMD_JUMP = b"J"

PAGE_SIZE = 128
BOOT_START_ADDRESS = 0x7C00
DEFAULT_BAUD = 9600


class IntelHexError(Exception):
    pass


@dataclass
class BootSender:
    ser: serial.Serial

    def read_for(self, duration_s: float) -> bytes:
        """Read available bytes for a fixed duration."""
        end_time = time.time() + duration_s
        rx = bytearray()

        while time.time() < end_time:
            chunk = self.ser.read(64)

            if chunk:
                rx.extend(chunk)

        return bytes(rx)

    def wait_for_ack(self, timeout_s: float = 1.0) -> bool:
        """Wait for ACK or NACK."""
        end_time = time.time() + timeout_s

        while time.time() < end_time:
            b = self.ser.read(1)

            if b == ACK:
                return True

            if b == NACK:
                return False

        return False

    def enter_update_mode(self, duration_s: float = 3.0) -> bool:
        """Enter bootloader command mode using U + P probe."""
        start_time = time.time()

        while time.time() - start_time < duration_s:
            self.ser.write(CMD_MAGIC)
            time.sleep(0.05)

            self.read_for(0.05)

            self.ser.write(CMD_PING)

            if self.wait_for_ack(timeout_s=0.2):
                return True

        return False

    def ping(self) -> bool:
        """Send ping command."""
        self.ser.write(CMD_PING)
        return self.wait_for_ack(timeout_s=1.0)

    def build_write_frame(self, page_addr: int, data: bytes) -> bytes:
        """Build one W frame."""
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
        """Write one flash page."""
        frame = self.build_write_frame(page_addr, data)

        self.ser.reset_input_buffer()
        self.ser.write(frame)

        return self.wait_for_ack(timeout_s=2.0)

    def jump_to_app(self) -> bool:
        """Send jump command."""
        self.ser.write(CMD_JUMP)
        return self.wait_for_ack(timeout_s=1.0)


def parse_hex_record(line: str) -> tuple[int, int, bytes]:
    """Parse one Intel HEX record."""
    line = line.strip()

    if not line:
        raise IntelHexError("empty line")

    if not line.startswith(":"):
        raise IntelHexError("missing ':' at record start")

    try:
        raw = bytes.fromhex(line[1:])
    except ValueError as exc:
        raise IntelHexError("invalid hex characters") from exc

    if len(raw) < 5:
        raise IntelHexError("record too short")

    byte_count = raw[0]
    address = (raw[1] << 8) | raw[2]
    record_type = raw[3]
    data = raw[4:4 + byte_count]

    if len(data) != byte_count:
        raise IntelHexError("record length mismatch")

    if len(raw) != byte_count + 5:
        raise IntelHexError("record size mismatch")

    if (sum(raw) & 0xFF) != 0:
        raise IntelHexError("record checksum failed")

    return record_type, address, data


def load_hex_pages(path: Path) -> list[tuple[int, bytes]]:
    """Load Intel HEX file and convert it into flash pages."""
    pages: dict[int, bytearray] = {}
    base_addr = 0

    with path.open("r", encoding="utf-8") as f:
        for line_number, line in enumerate(f, start=1):
            line = line.strip()

            if not line:
                continue

            try:
                record_type, address, data = parse_hex_record(line)
            except IntelHexError as exc:
                raise IntelHexError(f"line {line_number}: {exc}") from exc

            if record_type == 0x00:
                absolute_addr = base_addr + address

                for offset, value in enumerate(data):
                    flash_addr = absolute_addr + offset

                    if flash_addr >= BOOT_START_ADDRESS:
                        raise IntelHexError(
                            f"address 0x{flash_addr:04X} reaches bootloader section"
                        )

                    page_addr = (flash_addr // PAGE_SIZE) * PAGE_SIZE
                    page_offset = flash_addr - page_addr

                    if page_addr not in pages:
                        pages[page_addr] = bytearray([0xFF] * PAGE_SIZE)

                    pages[page_addr][page_offset] = value

            elif record_type == 0x01:
                break

            elif record_type == 0x02:
                if len(data) != 2:
                    raise IntelHexError("invalid extended segment address record")

                base_addr = (((data[0] << 8) | data[1]) << 4)

            elif record_type == 0x04:
                if len(data) != 2:
                    raise IntelHexError("invalid extended linear address record")

                base_addr = (((data[0] << 8) | data[1]) << 16)

            elif record_type in (0x03, 0x05):
                pass

            else:
                raise IntelHexError(f"unsupported record type 0x{record_type:02X}")

    if not pages:
        raise IntelHexError("no data records found")

    return [
        (page_addr, bytes(page_data))
        for page_addr, page_data in sorted(pages.items())
    ]


def make_test_page(seed: int) -> bytes:
    """Generate deterministic test data."""
    return bytes(((i + seed) & 0xFF) for i in range(PAGE_SIZE))


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", required=True)
    parser.add_argument("--baud", type=int, default=DEFAULT_BAUD)
    parser.add_argument("--addr", type=lambda x: int(x, 0), default=0x7B80)
    parser.add_argument("--pages", type=int, default=1)
    parser.add_argument("--hex", type=Path, help="Intel HEX firmware file")
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

        if args.hex:
            try:
                pages = load_hex_pages(args.hex)
            except IntelHexError as exc:
                print(f"HEX load failed: {exc}")
                return

            print(f"Loaded {len(pages)} page(s) from {args.hex}")

            for page_addr, page_data in pages:
                ok = sender.write_page(page_addr, page_data)

                if not ok:
                    print(f"Write failed at 0x{page_addr:04X}")
                    return

                print(f"Write OK at 0x{page_addr:04X}")

        else:
            for page_index in range(args.pages):
                page_addr = args.addr + page_index * PAGE_SIZE
                page_data = make_test_page(seed=page_index)

                ok = sender.write_page(page_addr, page_data)

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