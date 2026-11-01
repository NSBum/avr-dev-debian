# ATtiny3217 onboard GPIO test

This bare-metal AVR-LibC project toggles PA4 every 500 ms on an ATtiny3217.
It is intended for a board whose onboard indicator is connected to PA4.

## Contract

- MCU: ATtiny3217, `-mmcu=attiny3217`, 24-pin package assumed
- Clock: internal OSC20M selected for 20 MHz by `FUSE.OSCCFG`, main prescaler disabled
- `F_CPU`/`CLK_PER`: 20 MHz
- LED/output: PA4 (`PORTA.PIN4_bm`)
- Timing: TCB0 periodic interrupt every 1 ms; PA4 toggles after 500 interrupts
- Programmer: Atmel-ICE over UPDI (`atmelice_updi`, AVRDUDE part `t3217`)

The project does not change fuses. Confirm that the device's oscillator fuse is
configured for 20 MHz before relying on the timing.

## Build

```sh
make
make size
```

Generated ELF and Intel HEX files are placed in `build/`.

Programming is deliberately explicit:

```sh
make identify
make flash
make verify
```

These commands assume the target is powered, grounded, and connected to the
Atmel-ICE UPDI output. Hardware execution has not been verified by the build.
