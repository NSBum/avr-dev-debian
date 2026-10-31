# AVR Firmware Development Conventions

These conventions apply to firmware developed within this repository, primarily targeting Microchip tinyAVR 1-series devices.

They establish consistent coding practices, project organization, and firmware architecture.

Device-specific requirements and hardware datasheets take precedence over these conventions.

## 1. Language and toolchain

- Use C11 with AVR-GCC extensions where necessary.
- Do not use C++ unless specifically requested by the user
- Use AVR-LibC and direct register-level peripheral access.
- Do not introduce Arduino frameworks or hardware abstraction libraries unless explicitly requested.
- Prefer standard C and AVR-LibC facilities over custom implementations.
- Compile with `-std=gnu11 -Os -Wall -Wextra`.
- Use `-Werror` for validation builds.
- Use exact MCU identifiers with `-mmcu`.
- Keep compiler and linker options in the project Makefile.

## 2. Source organization

Use the following structure for substantial projects:

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

Small experimental programs may use a simpler layout.

- Keep generated files under `build/`.
- Do not commit generated binaries or object files.
- Use `.c` for implementation and `.h` for public interfaces.
- Avoid unnecessary source files or abstractions.

## 3. Naming conventions

Use `snake_case` for functions and variables:

```c
void uart_init(void);
uint8_t sensor_status;
```

Use `UPPER_SNAKE_CASE` for macros and compile-time constants:

```c
#define UART_BAUD_RATE 115200UL
#define RX_BUFFER_SIZE 64U
```

Use `_t` suffixes for project-defined types:

```c
typedef enum {
    HEATER_OFF,
    HEATER_ON
} heater_state_t;
```

Use `static` for functions and variables private to a translation unit.

Avoid global symbols unless they are intentionally part of a module's interface.

## 4. Types and arithmetic

- Prefer `<stdint.h>` fixed-width integer types for hardware-related values.
- Use `bool` from `<stdbool.h>` for logical state.
- Use unsigned types for bit masks and nonnegative counters.
- Use `size_t` for array lengths and object sizes where appropriate.
- Make integer promotion and overflow behaviour explicit.
- Use wider intermediate types for timer, baud-rate, and scaling calculations.
- Avoid floating-point arithmetic unless justified.
- Never assume `int` is 32 bits; AVR-GCC normally uses 16-bit `int`.

Example:

```c
uint32_t ticks =
    ((uint32_t)F_CPU * interval_ms) / 1000UL;
```

Check intermediate ranges before applying this pattern to large intervals.

## 5. Constants and configuration

- Prefer `static const` objects for typed constants where appropriate.
- Use macros when compile-time expressions are required by register configuration or conditional compilation.
- Keep board-specific pin assignments in `board.h`.
- Keep application-level configurable constants in `config.h`.
- Define `F_CPU` in the build system.
- Do not redefine `F_CPU` independently in multiple source files.
- Do not hardcode clock frequencies in peripheral drivers.

## 6. Register access

- Use register and bit definitions from the installed device headers.
- Prefer named bit masks and enumerations over numeric literals.
- Use `DIRSET`, `DIRCLR`, `OUTSET`, `OUTCLR`, and `OUTTGL` where supported.
- Preserve unrelated register fields when modifying configuration.
- Respect write-one-to-clear interrupt flags.
- Use `_PROTECTED_WRITE()` for CCP-protected registers.
- Do not create generic register wrappers unless they provide a demonstrated benefit.

## 7. Initialization

Initialize hardware in a deliberate order:

1. Establish safe GPIO output states.
2. Configure the system clock.
3. Initialize required peripherals.
4. Initialize application state.
5. Enable peripheral interrupts.
6. Enable global interrupts when required.
7. Enter the main application loop.

Adapt this sequence when hardware requires a different order.

Do not enable interrupts before their associated state and handlers are ready.

## 8. Main loop and state machines

Prefer an explicit main loop with bounded operations:

```c
int main(void)
{
    system_init();

    while (1) {
        application_process();
    }
}
```

- Prefer event-driven state machines over long blocking delays.
- Keep state transitions explicit.
- Use hardware timers for periodic scheduling.
- Avoid unnecessary polling at high frequency.
- Do not introduce an RTOS unless the project requires one.

## 9. Interrupts and shared state

- Keep ISRs short.
- Do not use blocking delays in ISRs.
- Avoid complex processing inside interrupt handlers.
- Use `volatile` for appropriate ISR-shared objects.
- Use atomic sections for multibyte shared accesses.
- Minimize the duration of disabled interrupts.
- Document ownership of shared state.
- Do not assume `volatile` provides atomicity or synchronization.

## 10. Error handling and timeouts

- Use explicit timeout policies for peripheral communication.
- Do not wait indefinitely for hardware flags unless explicitly justified.
- Check and handle peripheral error conditions.
- Avoid silently discarding communication errors.
- Prefer error codes or status enums over ambiguous return values.
- Define safe behaviour when communication or sensors fail.

## 11. Memory management

- Prefer static or automatic allocation.
- Avoid heap allocation unless justified.
- Avoid recursion.
- Keep stack usage predictable.
- Use bounded arrays and buffers.
- Check buffer indices and lengths.
- Consider SRAM consumption when allocating communication buffers.
- Store large immutable tables in program memory when beneficial and compatible with the selected device.

## 12. Comments and documentation

Comments should explain *why*, not restate obvious operations.

Good:

```c
// The prescaler must be disabled before USART baud configuration.
```

Unnecessary:

```c
// Set PA3 high.
PORTA.OUTSET = PIN3_bm;
```

Document:

- Non-obvious hardware requirements.
- Timing assumptions.
- Peripheral limitations.
- Errata workarounds.
- Safety-critical behaviour.
- Reasons for unusual implementation choices.

Use datasheet section references for hardware-dependent decisions where helpful.

## 13. Header files

- Use include guards or `#pragma once` consistently.
- Include only required headers.
- Keep public interfaces minimal.
- Do not define non-static global objects in headers.
- Avoid unnecessary dependencies between modules.
- Declare public functions with explicit parameter types.

Example:

```c
#ifndef UART_H
#define UART_H

#include <stdint.h>

void uart_init(uint32_t baud);
bool uart_write_byte(uint8_t value);

#endif
```

Include `<stdbool.h>` when using `bool` in a public header.

## 14. Build and verification

Every project should provide, where applicable:

- `make` — build firmware.
- `make clean` — remove generated files.
- `make size` — report flash and SRAM consumption.
- `make disasm` — generate assembly disassembly.
- `make flash` — explicitly program the target.
- `make verify` — verify programmed flash.

Builds must not automatically flash hardware.

Do not automatically write fuses or EEPROM as part of ordinary firmware builds.

Use the actual target MCU when reporting memory consumption.

## 15. Portability

- Do not assume all tinyAVR 1-series devices have identical peripherals.
- Isolate board-specific configuration.
- Avoid unnecessary device-specific conditional compilation inside generic drivers.
- Prefer separate configuration definitions when devices differ.
- Validate each explicitly supported target.
- Do not sacrifice correctness for superficial portability.

## 16. Version control

- Commit source code, Makefiles, documentation, and configuration.
- Exclude generated build artifacts.
- Keep commits focused on logical changes.
- Do not commit temporary debugging output.
- Preserve working firmware when experimenting with major changes.

## 17. Hardware safety

- Never change fuses without explicit authorization.
- Preserve UPDI programming access.
- Verify GPIO electrical constraints before configuring outputs.
- Avoid output contention.
- Establish safe states for relays, MOSFETs, heaters, and other controlled loads.
- Consider watchdog, brown-out, and power-loss behaviour.
- Never assume a successful compile proves electrical safety.

## 18. Definition of done

Firmware changes are complete only when:

1. The intended behaviour is implemented.
2. The affected targets compile successfully.
3. Compiler warnings are resolved.
4. Flash and SRAM usage are checked.
5. Relevant timing and concurrency concerns are reviewed.
6. Hardware-dependent assumptions are documented.
7. Any unperformed physical tests are identified explicitly.

Do not claim hardware verification unless the firmware was actually tested on the device.
