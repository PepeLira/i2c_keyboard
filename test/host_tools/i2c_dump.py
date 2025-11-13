#!/usr/bin/env python3
"""Herramienta de depuración para el teclado RP2040 por I²C."""

import argparse
import time

from smbus2 import SMBus

REG_DEV_ID = 0x00
REG_FW_VERSION = 0x01
REG_STATUS = 0x02
REG_MOD_MASK = 0x03
REG_FIFO_COUNT = 0x04
REG_FIFO_POP = 0x05
REG_CFG_FLAGS = 0x06
REG_CURSOR_STATE = 0x07
REG_LED_STATE = 0x08
REG_SCAN_RATE = 0x09
REG_TS_HIGH = 0x0A

EVENT_NAMES = {
    0x00: "NOP",
    0x01: "KEY_DOWN",
    0x02: "KEY_UP",
    0x03: "MOD_CHANGE",
    0x04: "CURSOR",
    0x05: "SPECIAL",
}


def read_register(bus: SMBus, address: int, reg: int, length: int = 1) -> bytes:
    return bytes(bus.read_i2c_block_data(address, reg, length))


def pop_event(bus: SMBus, address: int) -> tuple[int, int, int, int]:
    data = read_register(bus, address, REG_FIFO_POP, 4)
    return data[0], data[1], data[2], data[3]


def dump_status(bus: SMBus, address: int) -> None:
    dev_id = read_register(bus, address, REG_DEV_ID, 1)[0]
    fw_major, fw_minor = read_register(bus, address, REG_FW_VERSION, 2)
    status = read_register(bus, address, REG_STATUS, 1)[0]
    mod_mask = read_register(bus, address, REG_MOD_MASK, 1)[0]
    fifo_count = read_register(bus, address, REG_FIFO_COUNT, 1)[0]
    cursor_state = read_register(bus, address, REG_CURSOR_STATE, 1)[0]
    led_state = read_register(bus, address, REG_LED_STATE, 3)
    scan_rate = read_register(bus, address, REG_SCAN_RATE, 1)[0]
    ts_high = int.from_bytes(read_register(bus, address, REG_TS_HIGH, 2), "little")

    print(f"Device ID       : 0x{dev_id:02X}")
    print(f"FW Version      : {fw_major}.{fw_minor}")
    print(f"Status          : 0x{status:02X}")
    print(f"Modifier mask   : 0x{mod_mask:02X}")
    print(f"FIFO count      : {fifo_count}")
    print(f"Cursor state    : 0x{cursor_state:02X}")
    print(f"LED state (GRB) : {list(led_state)}")
    print(f"Scan rate (Hz)  : {scan_rate}")
    print(f"Timestamp hi    : 0x{ts_high:04X}")


def consume_events(bus: SMBus, address: int, limit: int | None) -> None:
    count = 0
    while True:
        event = pop_event(bus, address)
        event_type, code, mod_mask, ts_low = event
        if event_type == 0x00:
            break
        name = EVENT_NAMES.get(event_type, f"UNK({event_type})")
        print(
            f"[{count:02}] {name:<10} code=0x{code:02X} mods=0x{mod_mask:02X} ts=0x{ts_low:02X}"
        )
        count += 1
        if limit is not None and count >= limit:
            break
        time.sleep(0.001)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bus", type=int, default=1, help="Bus I²C a utilizar")
    parser.add_argument("--address", type=lambda v: int(v, 0), default=0x32, help="Dirección del dispositivo")
    parser.add_argument("--events", type=int, default=0, help="Cantidad de eventos a consumir (0 = todos)")
    args = parser.parse_args()

    with SMBus(args.bus) as bus:
        dump_status(bus, args.address)
        if args.events != 0:
            limit = None if args.events < 0 else args.events
            print("\nEventos:")
            consume_events(bus, args.address, limit)


if __name__ == "__main__":
    main()
