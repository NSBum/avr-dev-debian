# ATtiny3217 alternating LED test

This bare-metal AVR-LibC test alternates PB5 and PB6 every 500 ms: PB5 is
on while PB6 is off, then their states swap. It uses TCB0's 1 ms periodic
interrupt and atomically toggles both output latches together.

## Contract

- MCU/package: ATtiny3217, 24-pin VQFN. PB5 is package pin 11 and PB6 is
  package pin 10.
- Clock: internal OSC20M at 20 MHz with the main prescaler disabled.
  `FUSE.OSCCFG` must already select 20 MHz; this project does not write fuses.
- Output pins: PB5 and PB6, active-high LEDs assumed. An active-low LED will
  appear inverted but still alternate correctly.
- Timing: 500 ms per LED state (one-second full cycle).
- Programmer: Atmel-ICE over UPDI (`atmelice_updi`), AVRDUDE part `t3217`.

## Build and program

```sh
make
make size
make identify
make flash
make verify
```

`flash` writes only program flash; it does not modify fuses or EEPROM.
