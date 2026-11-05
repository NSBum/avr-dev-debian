# LD2410C presence indicator

Bare-metal ATtiny3217 firmware that reads the LD2410C radar report stream on
USART0 and exposes the DFPlayer through a documented I2C register interface.

Behavior:

- Radar movement never triggers playback.
- DFPlayer commands, folder/file indexes, volume, raw query commands, and
  feedback requests are controlled by I2C writes.

The default LD2410C report format is parsed as a streaming binary protocol.
The basic report payload contains target state, moving distance, and
stationary distance. State `0x03` (moving and stationary) is treated as moving
when the moving distance qualifies; otherwise its stationary distance is used.

## I2C interface

TWI0 operates as an I2C slave on the default PB1/SDA and PB0/SCL pins. The
address is `0x30` plus PC4 as bit 0 and PC5 as bit 1, so the selectable address
range is `0x30` through `0x33`. PC4 and PC5 require external pulldowns when
their high state is selected.

The radar and DFPlayer register maps are documented in
[`hardware.md`](hardware.md). The generic DFPlayer command registers pass the
native command byte and 16-bit parameter through, including commands without
convenience wrappers.

## DFPlayer wiring

- PA5: software UART TX to DFPlayer RX
- PA6: software UART RX from DFPlayer TX
- PA7: DFPlayer BUSY input

## Build

```sh
make
make size
```

Generated ELF and Intel HEX files are placed under `build/`. Programming is
explicit and not performed by the normal build:

```sh
make identify
make flash
make verify
```

## Hardware

See [`hardware.md`](hardware.md) for the MCU, pin, power, UART, and UPDI
assumptions. The LD2410C default UART is 256000 baud, 8-N-1.
