# ATtiny3217 serial output test

This bare-metal AVR-LibC project transmits a short status line once per second:

```text
ATtiny3217 serial OK
```

Serial format is 9600 baud, 8 data bits, no parity, and 1 stop bit (8-N-1).

## Contract

- MCU: ATtiny3217, `-mmcu=attiny3217`, 24-pin VQFN package
- Clock: internal OSC20M selected for 20 MHz by `FUSE.OSCCFG`, main prescaler disabled
- USART: USART0 default route, PB2/TXD (physical pin 14)
- Baud: 9600, normal asynchronous mode, `USART0.BAUD = 8333`
- Programmer: Atmel-ICE over UPDI (`atmelice_updi`, AVRDUDE part `t3217`)
- USB serial adapter: FT232 UART; connect MCU PB2/TXD to FT232 RXD and share ground

The project does not change fuses. Confirm that the device oscillator fuse is
configured for 20 MHz before relying on the baud rate.

## Build and program

```sh
make
make size
make identify
make flash
make verify
```

Generated ELF and Intel HEX files are placed in `build/`.

## Host capture

Find the FT232 device, usually `/dev/ttyUSB0` or a stable path under
`/dev/serial/by-id/`, then capture at 9600 8-N-1:

```sh
stty -F /dev/ttyUSB0 9600 cs8 -cstopb -parenb -ixon -ixoff raw
timeout 5s cat /dev/ttyUSB0
```

Reset or power-cycle the MCU after starting capture to see the first line.
