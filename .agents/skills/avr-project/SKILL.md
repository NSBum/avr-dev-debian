---
name: avr-project
description: Establish and maintain bare-metal AVR project foundations, including target MCU selection, clock speed, programmer settings, build configuration, and a consistent project layout. Use when starting, restructuring, or reviewing an AVR firmware project; use device-specific firmware skills for peripheral implementation and programming skills for actual flashing.
---

# AVR Project Setup

Use this skill to turn the hardware and toolchain requirements into an explicit, reproducible project contract. Do not begin by guessing an MCU, clock, or programmer from a nearby project.

## Establish the project contract

Before creating or changing project files, identify and record:

- Exact MCU part number and package, including the AVR family.
- Installed compiler/toolchain and the exact `-mmcu` value.
- Operating voltage and any board-level constraints that affect clock or peripherals.
- CPU clock source, nominal frequency, prescaler, and effective `F_CPU`/`CLK_PER` values.
- Programmer/debugger, physical programming interface, AVRDUDE programmer identifier, and target connection assumptions.
- Flash, SRAM, EEPROM, bootloader, fuse, and UPDI/RESET constraints that affect the build or deployment workflow.

If a requirement is unknown, inspect the existing project documentation and installed toolchain first. If it cannot be established safely, leave it as an explicit TODO or ask the user; do not silently substitute a similar AVR.

## Select the processor speed deliberately

Treat these as separate values and document their relationship:

- Oscillator or external clock frequency.
- Clock prescaler and effective CPU/peripheral clock.
- `F_CPU` used by AVR-LibC and application calculations.
- Peripheral-specific clock domains and their prescalers.

Choose the speed from the application requirements: timing resolution, UART baud error, PWM/timer ranges, power consumption, startup behavior, oscillator accuracy, and whether an external crystal or clock is actually available. Keep the initial clock configuration simple and compatible with the board. Whenever a clock affects a baud rate, timer period, delay, or timeout, keep the calculation visible in `config.h`, the Makefile, or `hardware.md`.

Do not imply that setting `F_CPU` changes the MCU clock. Firmware initialization, fuse settings, and compile-time constants must agree. Never change clock-related fuses or other persistent configuration without explicit user approval.

## Select and document the programmer

Keep programming configuration separate from application configuration. The project should make it easy to see:

- Programmer tool and interface, such as AVRDUDE over UPDI, ISP, or another supported interface.
- Programmer identifier and exact target part identifier.
- Connection assumptions: target power, ground, signal wiring, voltage levels, and RESET/UPDI ownership.
- Build artifact to flash, normally an Intel HEX generated from the ELF.

The Makefile may provide `program`, `verify`, and `identify` targets, but programming must remain an explicit user action. Do not override a device-signature mismatch, erase a chip, alter fuses, or disable/repurpose UPDI without explicit approval. Prefer read-only identification and build checks before any programming command.

## Use this project structure

For a small or medium bare-metal AVR project, use this layout unless the user or an existing repository convention requires a justified variation:

```text
project/
├── Makefile
├── README.md
├── hardware.md
├── include/
│   ├── board.h
│   └── config.h
├── src/
│   ├── main.c
│   └── ...
└── build/
```

Responsibilities:

- `Makefile`: one source of truth for `MCU`, `F_CPU`, compiler flags, output names, and programmer settings; provide at least `all`, `clean`, and size/reporting targets, plus deployment targets when requested.
- `README.md`: purpose, prerequisites, build commands, supported board/MCU, and concise programming instructions.
- `hardware.md`: schematic-level assumptions, power, clock source, pin assignments, programming header/wiring, alternate functions, and board revisions.
- `include/config.h`: compile-time application and clock configuration that belongs to firmware; avoid duplicating values already owned by the Makefile unless the relationship is explicit.
- `include/board.h`: board-specific pins, active levels, and physical resource mappings; keep generic drivers independent of board names where practical.
- `src/`: application and driver implementation, with initialization separated from the main application loop.
- `build/`: generated objects, dependency files, ELF, map, HEX, and size reports. It should be reproducible and normally ignored by version control.

Do not hide hardware assumptions in source files or shell history. Put the decision in the appropriate project file and make the build output reveal the selected MCU and clock.

## Makefile baseline

Use AVR-GCC and AVR-LibC conventions appropriate to the selected MCU. The baseline should:

- Pass the exact `-mmcu=$(MCU)` to compilation and linking.
- Define `F_CPU` consistently with the firmware clock configuration.
- Compile with useful warnings such as `-Wall -Wextra`, sensible optimization, debug information when needed, and dependency generation.
- Link to an ELF, derive HEX from that ELF, and report flash/SRAM usage where available.
- Keep generated files under `build/` and avoid relying on the caller's current directory.
- Make programmer and deployment variables overridable without editing source, while retaining safe project defaults.

Validate the project in layers: inspect the contract and file layout, compile with the exact MCU, link and inspect size, generate the HEX, then identify/program/verify hardware only when requested and available. Report precisely which layers were completed; a successful build is not evidence that the hardware ran the firmware.

For register-level work, apply the `attiny1-firmware` skill when it matches the device. For device identification, flashing, verification, or programmer faults, apply the `avr-programming` skill as well.
