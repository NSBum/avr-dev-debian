# Agent-assisted AVR development on Debian Linux

This repository provides a small, device-specific development environment for
Microchip tinyAVR 1-series firmware. It contains agent instructions,
engineering references, and checks for building and programming AVR devices
with AVR-GCC, AVR-LibC, AVRDUDE, and an Atmel-ICE over UPDI.

## Scope

The firmware guidance covers these tinyAVR 1-series devices:

`ATtiny212`, `214`, `412`, `414`, `416`, `417`, `814`, `816`, `817`,
`1614`, `1616`, `1617`, `3216`, and `3217`.

Device capabilities, pin mappings, registers, interrupt vectors, and fuse
behavior vary. The exact device datasheet and applicable errata are
authoritative. Installed AVR-GCC/AVR-LibC headers and the installed AVRDUDE
configuration are the next sources to check.

## Agent skills and references

The instructions used for agent-assisted work are in [`.agents/skills`](.agents/skills):

- [`attiny1-firmware`](.agents/skills/attiny1-firmware/SKILL.md) — device
  selection, clock and peripheral setup, register-level C, concurrency, and
  proportionate validation.
- [`avr-programming`](.agents/skills/avr-programming/SKILL.md) — target
  identification, UPDI programming, flash verification, USB diagnostics, and
  fuse safety.

Firmware references cover:

- [device family and toolchain identifiers](.agents/skills/attiny1-firmware/references/device-family.md)
- [clocks and CCP-protected writes](.agents/skills/attiny1-firmware/references/clocks-and-ccp.md)
- [GPIO and PORTMUX](.agents/skills/attiny1-firmware/references/gpio-and-portmux.md)
- [timers and interrupts](.agents/skills/attiny1-firmware/references/timers-and-interrupts.md)
- [USART, SPI, and TWI/I2C](.agents/skills/attiny1-firmware/references/serial-peripherals.md)
- [firmware code review](.agents/skills/attiny1-firmware/references/code-review.md)

Programming references cover [AVRDUDE commands](.agents/skills/avr-programming/references/avrdude-commands.md),
[fuse management](.agents/skills/avr-programming/references/fuse-management.md),
and [Atmel-ICE/UPDI troubleshooting](.agents/skills/avr-programming/references/troubleshooting.md).

## Development workflow

1. Establish the exact MCU, package, board wiring, clock configuration, pin
   routes, and ownership of peripherals and interrupts.
2. Check the target datasheet and installed device header before selecting
   registers, masks, vectors, or peripheral instances.
3. Build with the matching GCC `-mmcu` identifier, explicit clock assumptions,
   and warnings enabled.
4. Inspect flash/SRAM usage and resolve correctness warnings. Perform
   disassembly or timing checks when the design depends on them.
5. If hardware is connected, identify the device before programming. Confirm
   the signature, write only the requested memory, and verify the result.

The development skill does not generalize classic AVR register conventions to
modern tinyAVR devices. It also requires explicit handling of CCP-protected
writes, PORTMUX routes, interrupt-shared state, timing calculations, and safe
startup outputs.

## Local checks

The toolchain smoke tests are under
[`projects/test-toolchain-install`](projects/test-toolchain-install):

```bash
cd projects/test-toolchain-install
./test-attiny1-device-avail.sh
./test-attiny1-compile-blinkc.sh
```

The compile check builds the blink example for six representative targets
with `-Wall -Wextra -Werror` and reports program/data usage. It does not
replace board-level testing.

## Programming safety

Programming is an explicit hardware operation. Before using AVRDUDE, confirm
USB access, target power, ground, UPDI wiring, the exact MCU, and the matching
firmware build. Do not override a device-signature mismatch, disable UPDI,
perform a chip erase, or modify fuses without explicit authorization.

Report hardware work precisely: programmer detected, target identified, flash
written, flash verified, and firmware observed running are separate results.

## Project conventions

Repository conventions are documented in [`docs/conventions.md`](docs/conventions.md).
