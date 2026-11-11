# AVR Firmware Development Instructions

## Environment

- Host: Debian 13, native x86-64 Linux.
- Toolchain: Debian-packaged AVR-GCC, AVR-LibC, Binutils, Make, and AVRDUDE.
- Primary targets: Microchip tinyAVR 1-series, including ATtiny1616, ATtiny3217, ATtiny412, ATtiny416, ATtiny816, and ATtiny3216.
- Language: C11 with AVR-GCC extensions where appropriate.
- Programming hardware: Atmel-ICE using UPDI.
- Development is command-line based. Do not require Atmel Studio, MPLAB X, Arduino, or PlatformIO.
- The complete supported tinyAVR 1-series device list and family-specific notes are maintained in [`attiny1-firmware`](.agents/skills/attiny1-firmware/references/device-family.md); always confirm the exact device before relying on a family-level assumption.

## Skill routing

Use the local skills for task-specific workflows:

- [`avr-project`](.agents/skills/avr-project/SKILL.md): establish or review a project’s MCU, clock, programmer, Makefile, and directory structure.
- [`attiny1-firmware`](.agents/skills/attiny1-firmware/SKILL.md): implement and validate bare-metal drivers and application firmware for tinyAVR 1-series devices. Read its relevant references and use the exact device datasheet.
- [`avr-programming`](.agents/skills/avr-programming/SKILL.md): identify, program, verify, and diagnose AVR targets with AVRDUDE and Atmel-ICE.
- [`avr-documentation-writing`](.agents/skills/avr-documentation-writing/SKILL.md): produce documentation for the project, including README.md and other explanatory documents intended for end-users and developers alike.

For a new project, establish the project contract first, then apply the device-firmware skill for register-level changes. Apply the programming skill only when the task involves the physical target or programmer.

## Engineering principles

1. Prioritize correctness, deterministic behaviour, and hardware safety over brevity.
2. Use bare-metal AVR-LibC and direct peripheral registers.
3. Consult the target's device headers and Microchip documentation before assuming a register, bit mask, interrupt vector, or peripheral capability exists.
4. Do not invent peripheral names or copy register definitions from unrelated AVR families.
5. Never assume all tinyAVR 1-series devices have identical pinouts or peripheral instances.
6. Keep firmware memory-efficient, but avoid premature optimization that compromises clarity.
7. Compile and test changes before reporting success.

## Source of truth and precedence

When information conflicts, use this order and investigate discrepancies rather than silently choosing one:

1. The exact MCU datasheet and applicable Microchip errata.
2. Installed device-specific AVR-GCC/AVR-LibC headers and official toolchain documentation.
3. The project’s `hardware.md`, `Makefile`, `include/config.h`, and board documentation.
4. The local skill references and general AVR documentation.

Project-specific hardware documentation and build settings override repository defaults. Do not generalize register names, pin mappings, clock behaviour, or peripheral availability from a related device.

## Clock configuration

- Repository default: 20 MHz CPU frequency, internal oscillator, no prescaler. A project’s explicit hardware contract may choose another clock when its board and timing requirements justify it.
- For projects using the repository default, define `F_CPU=20000000UL` consistently in the build system. Projects with another justified clock must define their selected value consistently instead.
- The oscillator's 16/20 MHz selection depends on the OSCCFG fuse.
- Do not assume disabling the prescaler changes a 16 MHz oscillator into a 20 MHz oscillator.
- Use `_PROTECTED_WRITE()` from `<avr/cpufunc.h>` for CCP-protected registers.
- Ensure the clock configuration occurs before timing-dependent peripheral initialization.
- Never modify clock fuses without explicit authorization.

## GPIO

- Prefer atomic GPIO operations through `DIRSET`, `DIRCLR`, `OUTSET`, `OUTCLR`, and `OUTTGL`.
- Use the appropriate `PINn_bm` masks from the target's device headers.
- Distinguish between physical pin numbers, port pins, and peripheral alternate-function assignments.
- Verify whether pins are reserved for UPDI, RESET, or other hardware functions.
- Avoid unnecessary read-modify-write operations on peripheral registers.

## Interrupts and concurrency

- Keep ISRs short and deterministic.
- Use `volatile` for data shared with interrupts when appropriate.
- Remember that `volatile` does not provide atomicity.
- Protect multibyte shared accesses using appropriate atomic sections.
- Explicitly enable required peripheral interrupts and global interrupts.
- Verify interrupt vector names against the installed device headers.
- Do not perform blocking delays inside ISRs.

## Timing and delays

- Use `<util/delay.h>` only for simple delays with compile-time clock assumptions.
- Prefer hardware timers for production timing and scheduling.
- Account for timer prescalers, counter widths, interrupt latency, and oscillator accuracy.
- Avoid long blocking delays in event-driven firmware.

## Memory and embedded constraints

- Avoid dynamic allocation unless explicitly justified.
- Prefer static allocation and bounded buffers.
- Avoid recursion.
- Minimize stack consumption.
- Check flash and SRAM usage after builds.
- Treat EEPROM endurance and power-loss behaviour explicitly.

## Build requirements

- Use Makefiles unless another build system is specifically requested.
- Build with `-Os -Wall -Wextra` and an appropriate C standard.
- Prefer `-Werror` for validation builds.
- Use `-mmcu` for the exact target device.
- Produce ELF and Intel HEX outputs.
- Provide targets for `all`, `clean`, `size`, and `flash` where appropriate.
- Keep generated files in a dedicated build directory.
- Do not commit build artifacts.

## Programming safety

- Use AVRDUDE with `atmelice_updi` for compatible UPDI targets.
- Verify the selected MCU before programming.
- Never write fuses, erase EEPROM, or alter UPDI configuration without explicit authorization.
- Do not flash physical hardware automatically merely because compilation succeeds.
- Keep fuse operations separate from normal build and flash operations.
- Do not use force-signature options to bypass device mismatches.

## Verification

After changes:

1. Compile the affected target.
2. Report warnings and errors.
3. Check flash and SRAM usage.
4. Inspect disassembly when correctness depends on instruction sequences, CCP timing, or interrupt behaviour.
5. Distinguish compilation success from actual hardware verification.
6. Never claim firmware was tested on hardware unless it was actually executed on the target.

Report completion using this minimum summary:

- Target contract: exact MCU/package, clock source and effective frequency, board, and programmer/interface.
- Build result: commands or targets run, warnings/errors, and generated ELF/HEX paths.
- Resource result: flash and SRAM usage, including any relevant size limits.
- Hardware result: programmer detection, target identification, flash/verify status, and whether the firmware actually executed on hardware.
- Remaining work: tests not run, assumptions still unverified, or physical checks required.

## Change boundaries

- Preserve unrelated user changes and existing project conventions.
- Do not overwrite source, hardware documentation, configuration, or build tooling outside the requested scope.
- Keep generated artifacts in the project build directory and do not commit them.
- Ask before changing fuse settings, pin assignments, clock architecture, programming interfaces, or performing destructive operations such as chip erase.
- Avoid broad refactors when a focused change is sufficient.

## Working style

- Make small, reviewable changes.
- Explain hardware assumptions explicitly.
- Preserve existing working functionality.
- Ask before changing fuse settings, pin assignments, clock architecture, or programming interfaces.
- Prefer precise technical explanations over generic embedded-development advice.

