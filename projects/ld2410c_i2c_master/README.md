# LD2410C I2C status master

This is a separate ATtiny3217 firmware project. It is the I2C master for the
`ld2410c_presence` serial-to-I2C device; it does not connect to the LD2410C
serial output directly.

The master polls I2C slave address `0x30` and reads register `0x00`:

- `0x01` moving: PA5 high
- `0x02` stationary: PA6 high
- `0x00` not detected: PA7 high

All other status outputs are low. A missing or timed-out slave is treated as
not detected. TWI0 uses default PB1/SDA and PB0/SCL at 100 kHz.

Every five seconds, the master also commands the slave to play a DFPlayer
file from folder `04`. The sequence is file `0001`, `0002`, `0003`, `0004`,
`0005`, then back to `0001`. The command uses the slave's folder/file
convenience registers in one I2C write:

```text
register 0x23, folder 0x04, file 0x01..0x05, execute 0x01
```

The first playback is scheduled five seconds after startup. A file index is
advanced only when the I2C write succeeds; if the slave is unavailable, the
same file is retried at the next interval.

## Build

```sh
make
make size
```

Generated ELF and Intel HEX files are placed under `build/`. Programming is
explicit:

```sh
make identify
make flash
make verify
```
