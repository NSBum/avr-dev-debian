# ATtiny3217 serial GPIO command test

This bare-metal AVR-LibC project accepts newline-terminated commands on
USART0 and controls GPIO A4. Valid commands receive an acknowledgement in the
form `ACK` followed by the exact command and `CRLF`.

## Commands

| Command | Action | Acknowledgement |
|---|---|---|
| `A4STA1` | Drive PA4 high | `ACKA4STA1` |
| `A4STA0` | Drive PA4 low | `ACKA4STA0` |
| `A4BLK1` | Blink PA4 with a 1000 ms full cycle, 50% duty | `ACKA4BLK1` |
| `A4BLK2` | Blink PA4 with a 750 ms full cycle, 50% duty | `ACKA4BLK2` |
| `A4BLK3` | Blink PA4 with a 500 ms full cycle, 50% duty | `ACKA4BLK3` |
| `A4BLK4` | Blink PA4 with a 250 ms full cycle, 50% duty | `ACKA4BLK4` |

Commands must be terminated with `LF` or `CRLF`. A blink command starts with
PA4 low, then toggles every half-cycle, producing a 50% duty cycle. Invalid or
unsupported input receives `ERR` followed by `CRLF`.

## Hardware contract

- MCU: ATtiny3217, `-mmcu=attiny3217`, 24-pin VQFN package
- Clock: internal OSC20M selected for 20 MHz by `FUSE.OSCCFG`, main prescaler disabled
- GPIO: PA4, physical pin 5
- USART0 default route: PB2/TXD, physical pin 14; PB3/RXD, physical pin 13
- Serial format: 9600 baud, 8 data bits, no parity, 1 stop bit (8-N-1)
- Programmer: Atmel-ICE over UPDI (`atmelice_updi`, AVRDUDE part `t3217`)

The project does not change fuses. Confirm that `FUSE.OSCCFG` selects 20 MHz.
Connect MCU PB2/TXD to the FT232 RXD, MCU PB3/RXD to FT232 TXD if commands
will be sent from the host, and connect grounds. Use compatible TTL/CMOS I/O
levels, not RS-232 voltage levels.

## Build and program

```sh
make
make size
make identify
make flash
make verify
```

Generated ELF and Intel HEX files are placed in `build/`.

## Host duplex interaction

The FT232 is a USB-to-TTL-UART adapter. Use the stable path below when it is
present; the exact serial number can differ on another host. The host settings
must match the firmware: 9600 baud, 8 data bits, no parity, one stop bit,
and no flow control.

```sh
PORT=/dev/serial/by-id/usb-FTDI_FT232R_USB_UART_A9EHDVJR-if00-port0
stty -F "$PORT" 9600 cs8 -cstopb -parenb \
    -ixon -ixoff -crtscts -echo raw
```

### One-terminal interaction

Run this in one terminal. It keeps one read/write handle open for the FT232,
displays received acknowledgements, and sends each completed line to the MCU:

```sh
exec 3<> "$PORT"
cat <&3 &
READER_PID=$!
cleanup() {
    kill "$READER_PID" 2>/dev/null
    exec 3>&- 3<&-
}
trap cleanup EXIT INT TERM
while IFS= read -r COMMAND; do
    printf '%s\n' "$COMMAND" >&3
done
```

Type a command such as `A4STA1`, `A4BLK3`, or `A4STA0`, then press Enter. The
trailing newline is sent to the MCU. Press Ctrl-D to end the input loop.

Keeping the descriptor open is important: opening and closing the device for
each command can interfere with some USB-UART TTY drivers and is not needed.

### Two-terminal interaction

Alternatively, use one terminal to receive acknowledgements:

```sh
cat "$PORT"
```

In a second terminal, configure the same `PORT` variable and send commands;
the trailing `LF` is required:

```sh
PORT=/dev/serial/by-id/usb-FTDI_FT232R_USB_UART_A9EHDVJR-if00-port0
printf 'A4STA1\n' > "$PORT"
```

Expected response:

```text
ACKA4STA1
```

The FT232 serial device may have a different stable path on another host;
inspect `/dev/serial/by-id/` rather than assuming `/dev/ttyUSB0`.
