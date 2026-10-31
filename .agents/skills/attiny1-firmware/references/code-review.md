
# Firmware Code Review

Review bare-metal C firmware for Microchip (Atmel) tinyAVR 1-series devices with emphasis on correctness, reliability, and hardware safety.

Prioritize actual defects over style preferences. Distinguish confirmed defects, plausible risks, and issues requiring hardware verification.

## 1. Target and toolchain

- Confirm the exact MCU and `-mmcu` compiler setting.
- Verify register names, bit masks, interrupt vectors, and peripheral availability against the target device header.
- Confirm `F_CPU` matches the configured clock frequency and prescaler.
- Check compiler optimization settings and assumptions about integer widths.
- Verify that device-specific behavior is not incorrectly generalized across the tinyAVR family.

## 2. Peripheral correctness

- Verify peripheral initialization order and required clock configuration.
- Check register access widths, bit masks, and preservation of unrelated bits.
- Verify CCP-protected register writes use the correct mechanism.
- Check PORT versus VPORT access and PORTMUX routing.
- Confirm pin direction, pull-ups, inversion, input sensing, and output state.
- Check for conflicts involving UPDI, RESET, alternate pin functions, and shared peripherals.
- Verify timer clock sources, prescalers, periods, compare values, and overflow behavior.
- Check interrupt enable flags, status flags, and peripheral-specific flag-clearing semantics.
- Verify ADC references, prescalers, acquisition timing, and result interpretation.
- Verify USART baud calculations, frame configuration, and error handling.
- Check peripheral behavior during sleep, reset, and clock changes.

Do not assume register semantics from another AVR family. Consult the device datasheet when uncertain.

## 3. Arithmetic and data representation

- Check integer overflow, underflow, truncation, and signed/unsigned conversions.
- Verify intermediate expression widths, not just destination types.
- Check multiplication before division for intermediate overflow.
- Check shifts for operand width, sign, and valid shift counts.
- Verify time calculations and counter rollover behavior.
- Prefer unsigned modular subtraction for elapsed-time comparisons.
- Check division by zero and invalid array indices.
- Verify integer promotions involving 8-bit and 16-bit types.
- Check assumptions about `int` being 16 bits on AVR.
- Verify fixed-point scaling, rounding, and unit conversions.

## 4. Interrupts and concurrency

- Confirm every enabled interrupt has an appropriate ISR.
- Check interrupt vector names against the selected device header.
- Keep ISRs bounded, short, and nonblocking.
- Check that ISRs do not perform unbounded loops or blocking I/O.
- Mark state shared between ISRs and foreground code `volatile` where required.
- Do not treat `volatile` as providing atomicity or synchronization.
- Check atomic access to shared 16-bit and wider objects on the 8-bit CPU.
- Protect multi-step read-modify-write operations against ISR interference.
- Use `ATOMIC_BLOCK` or equivalent interrupt-state-preserving protection where necessary.
- Check shared ring-buffer indices, counters, and state-machine transitions.
- Verify that event flags cannot be lost through read/clear races.
- Consider interrupt latency, priority, nesting assumptions, and worst-case execution time.

## 5. Memory and stack

- Check static RAM consumption against the device's SRAM capacity.
- Review large local arrays, recursion, and deeply nested call chains.
- Estimate worst-case stack usage, including interrupt stack frames.
- Check for stack/heap collision and excessive automatic storage.
- Avoid dynamic allocation unless its behavior is explicitly controlled.
- Verify buffer sizes, termination, indexing, and bounds checks.
- Check use of flash-resident constant data versus SRAM-resident data.
- Verify EEPROM write limits, address bounds, and persistence assumptions.
- Inspect linker memory-usage output when available.

## 6. Control flow and reliability

- Check initialization before use and all reset/startup assumptions.
- Review state-machine transitions and unreachable or missing states.
- Verify timeout behavior, including timer rollover.
- Check error recovery from peripheral failures and invalid inputs.
- Confirm watchdog configuration and servicing cannot conceal deadlocks.
- Check behavior during brownout, power interruption, and reset.
- Verify that output pins enter safe states during startup and failures.
- Check blocking operations for unintended effects on timing or responsiveness.
- Identify undefined behavior and implementation-dependent assumptions.

## 7. Compiler and build validation

- Build for the exact target MCU with the intended optimization settings.
- Enable and review warnings, preferably `-Wall -Wextra`.
- Consider `-Wconversion` and `-Wsign-conversion` for targeted review.
- Investigate every warning; do not suppress warnings without justification.
- Check for implicit declarations, incompatible types, and missing prototypes.
- Check optimization-sensitive code, particularly ISR-shared state.
- Inspect generated assembly when timing, atomicity, or register access is critical.
- Use `avr-size` to review flash and SRAM consumption.
- Use `avr-objdump` when verifying interrupt vectors, initialization, or critical instruction sequences.
- Do not claim successful compilation or testing unless actually performed.

## 8. Hardware verification

Identify behavior that cannot be established from source review alone.

- Confirm physical pin assignments against the package pinout and schematic.
- Verify voltage levels, drive current, pull-ups, and external loading.
- Check clock-source accuracy and timing assumptions.
- Verify timer outputs and interrupt timing with appropriate instruments.
- Check USART signals, baud rate, framing, and electrical interface.
- Verify reset, UPDI programming, and power sequencing.
- Consider noise, contact bounce, signal integrity, and asynchronous inputs.
- Test boundary conditions, timeouts, counter rollover, and fault recovery.
- Distinguish simulated or calculated results from measurements on hardware.

## 9. Review output

Report findings in descending severity:

- **Critical:** Possible hardware damage, memory corruption, or unsafe behavior.
- **High:** Confirmed functional defects, races, or incorrect peripheral configuration.
- **Medium:** Plausible reliability, timing, or portability problems.
- **Low:** Maintainability improvements or defensive checks.

For each finding, provide:

1. File and line number, when available.
2. Specific defect or risk.
3. Technical explanation grounded in code or device behavior.
4. Consequence or realistic failure scenario.
5. Minimal recommended correction.
6. Whether the finding is confirmed or requires verification.

Avoid speculative warnings without a plausible failure mechanism.

If no defects are found, state that explicitly and identify any remaining validation gaps.

Do not modify firmware during a review unless explicitly requested.

