# Microchip tinyAVR 1-Series Device Family

## Purpose

This reference defines the supported Microchip tinyAVR 1-series microcontrollers and the rules for selecting, configuring, and validating firmware for individual devices.

It applies exclusively to the tinyAVR 1-series, not the classic ATtiny family, tinyAVR 0-series, tinyAVR 2-series, or AVR Dx family.

The exact device datasheet, device-specific errata, and installed toolchain definitions take precedence over generic family assumptions.

## 1. Complete supported device list

The tinyAVR 1-series consists of the following 14 devices.

| Device | Flash | Pins | GCC `-mmcu` | AVRDUDE `-p` |
|---|---:|---:|---|---|
| ATtiny212 | 2 KB | 8 | `attiny212` | `t212` |
| ATtiny214 | 2 KB | 14 | `attiny214` | `t214` |
| ATtiny412 | 4 KB | 8 | `attiny412` | `t412` |
| ATtiny414 | 4 KB | 14 | `attiny414` | `t414` |
| ATtiny416 | 4 KB | 20 | `attiny416` | `t416` |
| ATtiny417 | 4 KB | 24 | `attiny417` | `t417` |
| ATtiny814 | 8 KB | 14 | `attiny814` | `t814` |
| ATtiny816 | 8 KB | 20 | `attiny816` | `t816` |
| ATtiny817 | 8 KB | 24 | `attiny817` | `t817` |
| ATtiny1614 | 16 KB | 14 | `attiny1614` | `t1614` |
| ATtiny1616 | 16 KB | 20 | `attiny1616` | `t1616` |
| ATtiny1617 | 16 KB | 24 | `attiny1617` | `t1617` |
| ATtiny3216 | 32 KB | 20 | `attiny3216` | `t3216` |
| ATtiny3217 | 32 KB | 24 | `attiny3217` | `t3217` |

These identifiers are the expected names for the supported toolchain. Verify availability in the installed versions of AVR-GCC and AVRDUDE before use.

Pin counts refer to device package variants, not the number of usable GPIO signals.

## 2. Device naming conventions

The part number generally encodes flash capacity and package pin count.

Examples:

- ATtiny1616: 16 KB flash, 20-pin package.
- ATtiny3217: 32 KB flash, 24-pin package.
- ATtiny412: 4 KB flash, 8-pin package.

Do not derive peripheral availability, SRAM capacity, EEPROM capacity, or pin assignments solely from the part number.

The device suffix and package designation must be checked when selecting physical pin assignments.

## 3. Family architecture

The tinyAVR 1-series uses the modern AVR architecture with memory-mapped peripheral registers.

Important characteristics include:

- 8-bit AVR CPU architecture.
- UPDI programming and debugging interface.
- PORT and VPORT GPIO interfaces.
- Configurable internal oscillator and main clock prescaler.
- CCP-protected system registers.
- Modern TCA and TCB timer peripherals, where implemented.
- USART, SPI, TWI, ADC, and other peripherals according to the exact device.
- Event System support according to the device.
- Device-specific interrupt vectors.
- Fuse-controlled system configuration.

Do not use classic AVR register conventions without checking the target device headers.

For example, do not assume `DDRB`, `PORTB`, `TCCR0A`, or `UBRR0H` are valid for these devices.

## 4. Device-specific peripheral availability

Not every tinyAVR 1-series device provides the same peripheral instances or capabilities.

Before implementing a driver, verify:

| Peripheral | Required checks |
|---|---|
| GPIO | Available ports, pins, input sensing, alternate functions |
| TCA | Presence, instance count, PWM modes, output routing |
| TCB | Instance count, clock sources, operating modes |
| RTC/PIT | Clock sources, periods, interrupt behaviour |
| USART | Instance count, pins, PORTMUX options, baud registers |
| SPI | Instance count, supported modes, physical routing |
| TWI | Instance count, controller/client capabilities, pins |
| ADC | Resolution, channels, references, sampling behaviour |
| AC | Comparator availability, input routing, outputs |
| DAC | Availability, resolution, output pin |
| CCL | Availability, LUT configuration, routing |
| EVSYS | Available channels, generators, users |
| WDT | Timeout configuration, protection, reset behaviour |
| NVMCTRL | Flash, EEPROM, and user-row operations |

Do not infer that a peripheral exists merely because another member of the family provides it.

A successful build for one device does not establish peripheral compatibility with another.

## 5. Clock configuration

The intended default project configuration is:

- Internal oscillator: nominal 20 MHz.
- Main clock prescaler: disabled.
- Build-time `F_CPU`: `20000000UL`.

This is a project convention, not a guarantee of the device's actual operating frequency.

The 16/20 MHz oscillator selection depends on the OSCCFG fuse.

The main clock prescaler is configured through CLKCTRL.

CCP protection applies to relevant clock-control registers.

Before using timing-dependent code:

1. Verify the oscillator configuration fuse.
2. Configure the main clock source.
3. Configure the prescaler.
4. Confirm that the resulting clock frequency is consistent with `F_CPU`.
5. Check peripheral-specific clock dividers.

Do not assume the device starts at 20 MHz with no prescaler after reset.

Consult `clocks-and-ccp.md` for implementation details.

## 6. GPIO and pin mapping

Physical package pin numbers are not interchangeable with port-bit numbers.

For each GPIO assignment, verify:

1. Exact MCU part number.
2. Physical package.
3. Port and bit designation.
4. Alternate peripheral functions.
5. PORTMUX configuration.
6. UPDI or RESET functionality.
7. Electrical loading and voltage limits.

Prefer atomic GPIO register operations where supported:

- `DIRSET`
- `DIRCLR`
- `OUTSET`
- `OUTCLR`
- `OUTTGL`

Do not assume every package exposes every port bit.

## 7. Interrupt compatibility

Interrupt vector names and peripheral instances must be verified against the selected device header.

Do not copy ISR names from classic AVR devices or another tinyAVR family.

When using interrupts:

- Confirm the peripheral instance exists.
- Confirm the exact interrupt vector.
- Configure the appropriate interrupt enable bits.
- Clear interrupt flags according to documented semantics.
- Protect shared multibyte data where necessary.
- Keep ISRs bounded and deterministic.

## 8. Memory constraints

Memory capacity varies substantially across the family.

The smallest devices have only 2 KB of flash, while the largest have 32 KB.

Firmware must be evaluated against the exact target's flash, SRAM, and EEPROM limits.

After building, inspect memory consumption:

```bash
avr-size -G --mcu=attiny1616 firmware.elf
```

Replace `attiny1616` with the actual target.

Pay particular attention to:

- Static and global variables.
- Stack usage.
- Interrupt nesting and call depth.
- Lookup tables and string literals.
- Buffer sizes.
- EEPROM endurance.
- Bootloader or reserved memory regions, if applicable.

Do not assume that successful compilation means the application has sufficient runtime stack space.

## 9. Verify compiler support

For a selected MCU:

```bash
avr-gcc -mmcu=attiny1616 -Os \
    -Wall -Wextra \
    -o firmware.elf main.c
```

To inspect which device header is selected:

```bash
printf '#include <avr/io.h>\n' |
    avr-gcc -mmcu=attiny1616 \
    -E -H -x c - -o /dev/null
```

Use the installed header definitions to verify:

- Register names.
- Bit masks.
- Peripheral structures.
- Interrupt vectors.
- Feature macros.

Do not manually redefine missing hardware registers merely to make compilation succeed.

If a required device definition is absent, investigate toolchain support or the device's actual capabilities.

## 10. Verify AVRDUDE support

List supported parts using the locally installed AVRDUDE:

```bash
avrdude -p '?'
```

For an ATtiny1616 connected through Atmel-ICE:

```bash
avrdude -c atmelice_updi -p t1616
```

The exact MCU must match the physical target.

Never override a mismatched signature.

For programming procedures, consult the separate `avr-programming` skill.

## 11. Cross-device portability

When writing firmware intended for multiple tinyAVR 1-series devices:

1. Identify the supported target set explicitly.
2. Verify peripheral availability on every supported device.
3. Separate board-specific pin assignments from peripheral logic.
4. Use compile-time configuration for legitimate hardware differences.
5. Avoid duplicating complete drivers unnecessarily.
6. Compile against every supported target.
7. Document features unavailable on particular devices.
8. Perform physical testing on representative hardware where possible.

Do not silently substitute a different peripheral when the requested one is unavailable.

Do not treat a successful compile-only test as proof of electrical or functional compatibility.

## 12. Device selection checklist

Before beginning a new firmware project, establish:

- [ ] Exact MCU part number
- [ ] Physical package and pinout
- [ ] Flash, SRAM, and EEPROM requirements
- [ ] Required peripheral instances
- [ ] GPIO assignments
- [ ] Clock source and prescaler
- [ ] Relevant fuse configuration
- [ ] Interrupt requirements
- [ ] UPDI accessibility
- [ ] Programmer and AVRDUDE compatibility
- [ ] Build and validation targets

If any critical hardware detail is unknown, identify the assumption explicitly and request clarification before making potentially unsafe changes.

## 13. Authoritative documentation

Use the following sources, in order of relevance:

1. Exact Microchip device datasheet.
2. Device-specific Microchip errata.
3. Installed AVR-LibC device header.
4. AVR-GCC documentation.
5. AVR-LibC documentation.
6. AVRDUDE documentation for programming operations.

Official resources:

- Microchip: https://www.microchip.com/
- AVR-LibC: https://www.nongnu.org/avr-libc/
- GCC AVR options: https://gcc.gnu.org/onlinedocs/gcc/AVR-Options.html
- AVRDUDE: https://avrdudes.github.io/avrdude/

Do not substitute documentation for a similarly named device without checking compatibility.
