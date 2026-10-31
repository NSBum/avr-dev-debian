# AVRDUDE Command Reference

## Environment

- Host: Debian 13
- Programmer: Atmel-ICE
- Interface: UPDI
- Default target: ATtiny1616
- AVRDUDE programmer ID: `atmelice_updi`
- AVRDUDE part ID: `t1616`

Always use the installed AVRDUDE version as the authority for supported programmer IDs, part IDs, and memory names.

## Inspect local capabilities

```bash
avrdude -v
avrdude -c '?'
avrdude -p '?'
```

For a specific device, inspect its supported memories:

```bash
avrdude -c atmelice_updi -p t1616 -v
```

Do not assume that memory names are identical across AVRDUDE versions.

## Identify a connected device

```bash
avrdude -c atmelice_updi -p t1616
```

This is the standard initial communication test.

A successful connection and matching device signature are required before programming.

## Write flash

```bash
avrdude -c atmelice_updi -p t1616 \
    -U flash:w:firmware.hex:i
```

AVRDUDE normally verifies flash after writing. Do not use `-V` to disable verification.

## Explicit flash verification

```bash
avrdude -c atmelice_updi -p t1616 \
    -U flash:v:firmware.hex:i
```

## Read flash

```bash
avrdude -c atmelice_updi -p t1616 \
    -U flash:r:flash-backup.hex:i
```

## Read EEPROM

```bash
avrdude -c atmelice_updi -p t1616 \
    -U eeprom:r:eeprom-backup.hex:i
```

## Write EEPROM

```bash
avrdude -c atmelice_updi -p t1616 \
    -U eeprom:w:eeprom.hex:i
```

EEPROM writes require explicit authorization.

## Read oscillator fuse

```bash
avrdude -c atmelice_updi -p t1616 \
    -U osccfg:r:-:h
```

Interpret the value using the exact device datasheet and installed AVRDUDE memory definitions.

## Safety requirements

- Never bypass signature checks using `-F`.
- Never disable verification using `-V`.
- Never change fuses without explicit authorization.
- Never erase the device without understanding EEPROM and configuration consequences.
- Never flash a device whose identity does not match the build target.
- Never assume a successful programming operation proves application functionality.
- Never guess an AVRDUDE command option or memory name when it can be checked locally.

