---
name: attiny1-firmware
description: Develop, modify, and validate bare-metal C firmware for Microchip tinyAVR 1-series devices with AVR-GCC and AVR-LibC. Use for GPIO, clocks, timers, USART, SPI, TWI/I2C, ADC, interrupts, and other peripheral drivers.
---

# tinyAVR 1-series firmware

Use this skill for firmware that targets a Microchip tinyAVR 1-series MCU. Work from the exact device and board constraints; do not generalize register names, capabilities, pin mappings, or interrupt vectors across the family.

## Supported devices

- ATtiny212, ATtiny214
- ATtiny412, ATtiny414, ATtiny416, ATtiny417
- ATtiny814, ATtiny816, ATtiny817
- ATtiny1614, ATtiny1616, ATtiny1617
- ATtiny3216, ATtiny3217

For device-specific capabilities, memory sizes, and toolchain identifiers, see [`references/device-family.md`](references/device-family.md). The device datasheet remains authoritative.

## Evidence hierarchy

When sources disagree, use this order:

1. The exact target MCU datasheet and applicable official errata.
2. Installed device-specific AVR-GCC/AVR-LibC headers, for symbols and compiler-supported definitions.
3. Official AVR-GCC and AVR-LibC documentation.
4. Repository hardware documentation and established project conventions.
5. The supplementary references in this skill.

Investigate disagreements between the datasheet and headers. Do not silently choose one, and do not invent registers, bit masks, peripheral capabilities, or interrupt vectors.

## References

Read the relevant reference when it contains guidance for the task, but use the datasheet for device behavior:

| Reference | Use for |
|---|---|
| [`device-family.md`](references/device-family.md) | Device selection and family differences |
| [`clocks-and-ccp.md`](references/clocks-and-ccp.md) | Clock setup and CCP-protected registers |
| [`gpio-and-portmux.md`](references/gpio-and-portmux.md) | GPIO and alternate peripheral routing |
| [`timers-and-interrupts.md`](references/timers-and-interrupts.md) | TCA, TCB, RTC, PWM, interrupts, and scheduling |
| [`serial-peripherals.md`](references/serial-peripherals.md) | USART, SPI, TWI/I2C, buffering, and baud rates |
| [`code-review.md`](references/code-review.md) | Firmware correctness, timing, safety, and concurrency review |

## Workflow

### 1. Establish the hardware contract

Before changing hardware-dependent code, identify:

- The exact MCU and package.
- Peripheral instances, physical pins, and PORTMUX routes.
- Clock sources, frequencies, prescalers, and timing assumptions.
- Existing ownership of peripherals, pins, interrupt vectors, event channels, buffers, and shared state.

Inspect the installed device header for the required symbols and confirm behavior in the datasheet. Do not silently reconfigure resources owned by another module.

### 2. Design the smallest safe configuration

For each peripheral:

- Verify availability, routing, clock requirements, reset state, and initialization order.
- Calculate register values from explicit clock and timing assumptions; document non-obvious calculations.
- Identify interrupt flags and their clearing semantics.
- Define error handling, timeout behavior, shutdown behavior, and safe output states.
- Decide how interrupt-shared state is synchronized.

Start with the smallest functional configuration, then add features only as required.

### 3. Implement according to register semantics

- Use device-specific register definitions, named masks, and fixed-width integer types where representation matters.
- Prefer atomic SET/CLR/TGL registers when provided. Use read-modify-write only when appropriate, and preserve reserved or unrelated fields.
- Respect write-one-to-clear and other special register semantics.
- Use `_PROTECTED_WRITE()` for CCP-protected writes and obey the protected-write timing window.
- Do not treat `volatile` as a substitute for atomic access or synchronization.
- Keep initialization, peripheral operation, and application logic separate.
- Avoid blocking operations in asynchronous drivers and avoid unnecessary register access from multiple execution contexts.
- Protect or explicitly handle USART framing, parity, and overflow errors, ADC failures, and communication failures where applicable.
- Do not accidentally reconfigure the UPDI pin.

### 4. Validate proportionally

When the build environment is available:

1. Compile with the exact `-mmcu` value and `-Wall -Wextra`.
2. Resolve warnings that indicate a correctness or portability problem.
3. Link successfully and report flash/SRAM usage when the build provides it.
4. For timing-critical code, inspect generated assembly or disassembly as needed.
5. If hardware is available, program the device and verify the flash contents.
6. Run the relevant functional tests and document what still requires physical testing.

Report verification precisely. Distinguish source inspection, compilation, linking, static/disassembly inspection, programming/flash verification, and hardware execution; never claim a level that was not completed.

## Common failure modes

- Applying classic AVR register names or assumptions to tinyAVR 1-series hardware.
- Using the wrong interrupt vector or assuming vectors are identical across devices.
- Assuming `F_CPU` matches the configured oscillator.
- Miscalculating timer periods or USART baud rates.
- Selecting the wrong PORTMUX route.
- Clearing an interrupt flag incorrectly or missing a required flag.
- Reading or writing multibyte interrupt-shared state non-atomically.
- Exceeding the CCP protected-write window.
- Taking ownership of a resource without checking the rest of the application.
