# AVR Toolchain Tests

Small bare-metal C examples and checks for validating an AVR-GCC/AVR-LibC installation against selected Microchip tinyAVR 1-series devices.

## Contents

- `projects/test-toolchain-install/blink.c` — a minimal LED blink program for the tinyAVR 1-series.
- `projects/test-toolchain-install/test-attiny1-compile-blinkc.sh` — compiles the blink program for each configured MCU and reports program/data usage.
- `projects/test-toolchain-install/test-attiny1-device-avail.sh` — checks whether the installed AVR-GCC headers and linker support each target.
- `docs/conventions.md` — project conventions.

## Supported test targets

The current test scripts cover:

`attiny1616`, `attiny3217`, `attiny412`, `attiny416`, `attiny816`, and `attiny3216`.

The source uses tinyAVR 1-series register definitions and is intended for an LED connected to `PA3`. It selects the internal 16/20 MHz oscillator, disables the main clock prescaler, and toggles the output every 500 ms. The effective clock frequency depends on the selected device's oscillator configuration.

## Requirements

- AVR-GCC with support for the target MCUs
- AVR-LibC, including `avr/io.h`, `avr/cpufunc.h`, and `<util/delay.h>`
- GNU Bash, `awk`, and `avr-size`

No programmer, board, or hardware connection is required to run the checks. The scripts only perform compilation and ELF inspection; they do not flash a device.

## Run the checks

From the example directory:

```sh
cd projects/test-toolchain-install
./test-attiny1-device-avail.sh
./test-attiny1-compile-blinkc.sh
```

The compile check builds with `-Os`, `-Wall`, `-Wextra`, `-Werror`, and GNU C11. It returns a non-zero exit status if any configured target fails.

To compile one target manually:

```sh
avr-gcc -mmcu=attiny1616 -DF_CPU=20000000UL \
  -Os -Wall -Wextra -Werror -std=gnu11 \
  -o /tmp/attiny1616-blink.elf blink.c
```

The target MCU, clock definition, pin assignment, and board wiring must agree before programming the resulting ELF or a converted HEX file.

