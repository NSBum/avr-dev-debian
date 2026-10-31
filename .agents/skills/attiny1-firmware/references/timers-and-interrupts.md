# Timers and Interrupts — tinyAVR 1-Series

## 1. Peripheral availability

Identify the exact MCU before selecting a timer, interrupt vector, or output pin.

| Devices | TCA0 | TCB0 | TCB1 | TCD0 | RTC/PIT |
|---|---|---|---|---|---|
| ATtiny212/214 | Yes | Yes | No | Yes | Yes |
| ATtiny412/414/416/417 | Yes | Yes | No | Yes | Yes |
| ATtiny814/816/817 | Yes | Yes | No | Yes | Yes |
| ATtiny1614/1616/1617 | Yes | Yes | Yes | Yes | Yes |
| ATtiny3216/3217 | Yes | Yes | Yes | Yes | Yes |

Peripheral capabilities:

- **TCA0:** 16-bit timer; periodic interrupts, frequency generation, PWM, and split mode.
- **TCB0/TCB1:** 16-bit timer; periodic interrupts, input capture, frequency measurement, timeout, and PWM modes.
- **TCD0:** 12-bit timer optimized for advanced waveform generation, complementary outputs, and dead-time control.
- **RTC:** 16-bit counter clocked independently of the main CPU clock.
- **PIT:** Periodic Interrupt Timer integrated into the RTC peripheral; useful for low-power wakeups.

TCA split mode provides two independent 8-bit counters with separate period and compare registers. It does not provide two independent 16-bit timers.

Timer instances, waveform outputs, interrupt vectors, and PORTMUX routing must be verified against the target device's datasheet and AVR-LibC header.

## 2. Timer selection

| Requirement | Preferred peripheral |
|---|---|
| General periodic system tick | TCB |
| Multiple PWM outputs | TCA |
| Two independent 8-bit timer periods | TCA split mode |
| Input pulse-width measurement | TCB capture |
| Complementary PWM/dead time | TCD |
| Low-power periodic wakeup | RTC/PIT |
| Long-duration scheduling | RTC or software counter driven by a periodic interrupt |

Prefer hardware PWM over software-generated PWM.

Prefer hardware capture or Event System routing when timing precision is important.

Do not allocate a timer already used by another subsystem without explicitly coordinating ownership.

## 3. Exact timing calculations

Always determine the actual peripheral clock, not merely the oscillator frequency.

Account for:

- Oscillator configuration and frequency tolerance.
- CPU clock prescaling.
- Timer-specific prescaling and clock source.
- Counting mode and TOP semantics.
- Interrupt latency and execution time.
- Integer rounding and counter width.

For a timer counting from zero through TOP:

    f_timer = f_source / prescaler

    f_interrupt = f_timer / (TOP + 1)

    TOP = (f_timer / f_interrupt) - 1

    period = (TOP + 1) / f_timer

These equations apply only when the selected timer mode resets the counter after TOP.

Example: TCB periodic interrupt at 1 kHz:

    CLK_PER      = 20,000,000 Hz
    TCB divider  = 2
    f_timer      = 10,000,000 Hz
    f_interrupt  = 1,000 Hz

    TOP = 10,000,000 / 1,000 - 1
        = 9,999

    Actual period = 1.000 ms

For non-integer results, calculate the actual achieved frequency and timing error:

    error_ppm = 1,000,000 *
                (actual_frequency / target_frequency - 1)

Use sufficiently wide arithmetic, preferably `uint64_t` for intermediate calculations that might overflow 32 bits.

Do not assume that all timer modes use `TOP + 1`. Verify PWM mode, count direction, and endpoint behavior.

## 4. TCA — Timer/Counter Type A

TCA supports:

- Normal 16-bit counting.
- Overflow and compare interrupts.
- Single-slope and dual-slope PWM.
- Split mode with two 8-bit counters.
- Multiple waveform output channels.

Important considerations:

- Normal and split modes use different register layouts and interrupt sources.
- Split mode reduces each counter's resolution to 8 bits.
- PWM frequency calculations depend on waveform generation mode.
- PWM outputs require correct PORTMUX routing and pin configuration.
- Buffered period and compare registers can prevent glitches during PWM updates.
- Interrupt flags are generally cleared by writing one to their corresponding bits.

For single-slope PWM using PER as TOP:

    f_PWM = f_TCA / (PER + 1)

For common dual-slope modes:

    f_PWM = f_TCA / (2 * PER)

Verify the precise formula for the selected waveform mode.

Never assume the compare register value maps directly to a percentage duty cycle without checking endpoint behavior and waveform polarity.

## 5. TCB — Timer/Counter Type B

TCB is particularly useful for periodic interrupts and input capture.

Common operating modes include:

- Periodic interrupt.
- Time-out check.
- Input capture.
- Frequency measurement.
- Pulse-width measurement.
- 8-bit PWM.

### Example: 1 ms periodic interrupt

Assumes:

- Target has TCB0.
- `CLK_PER` is configured to 20 MHz.
- TCB clock is `CLK_PER / 2`.
- No other subsystem owns TCB0.

```c
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <util/atomic.h>

static volatile uint32_t ticks_ms;

void timer_init(void)
{
    TCB0.CTRLA = 0;

    TCB0.CTRLB = TCB_CNTMODE_INT_gc;
    TCB0.CCMP = 9999;

    TCB0.INTFLAGS = TCB_CAPT_bm;
    TCB0.INTCTRL = TCB_CAPT_bm;

    TCB0.CTRLA = TCB_CLKSEL_CLKDIV2_gc
               | TCB_ENABLE_bm;
}

ISR(TCB0_INT_vect)
{
    TCB0.INTFLAGS = TCB_CAPT_bm;
    ++ticks_ms;
}

uint32_t millis_get(void)
{
    uint32_t snapshot;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        snapshot = ticks_ms;
    }

    return snapshot;
}
```

Call `timer_init()` before enabling global interrupts with `sei()`.

The TCB periodic mode uses the CCMP register as the period limit. Do not assume the same CCMP semantics apply in capture or PWM modes.

Clear pending interrupt flags before enabling the peripheral.

The 32-bit tick counter wraps after approximately 49.7 days. Design elapsed-time calculations to handle wraparound.

## 6. RTC and PIT

The RTC provides a clock domain independent of the main CPU clock.

Potential sources include an internal low-frequency oscillator and supported external 32.768 kHz sources.

PIT provides fixed periodic interrupt intervals selected by hardware configuration.

Typical PIT periods are powers-of-two clock cycles, such as 4, 8, 16, through 32768 cycles, depending on device configuration.

At 32768 Hz:

    32768 cycles = 1 second
    1024 cycles  = 31.25 ms
    32 cycles    = 0.9765625 ms

Important:

- PIT periods are selected from predefined divisors, not arbitrary compare values.
- RTC registers may require synchronization.
- Check synchronization-busy flags before dependent register operations.
- RTC/PIT interrupt flags must be acknowledged correctly.
- Verify oscillator availability and accuracy requirements.
- RTC/PIT can continue operating in supported sleep modes where synchronous CPU timers cannot.

Do not describe an internal low-frequency oscillator as an accurate real-time clock without considering calibration and temperature drift.

## 7. Interrupt controller and priorities

tinyAVR 1-series devices use CPUINT rather than the classic ATmega interrupt-priority model.

Priority structure:

1. Non-maskable interrupts, where implemented.
2. Level 1 interrupt: one optionally selected interrupt vector.
3. Level 0 interrupts: remaining maskable vectors.

Within Level 0, priority normally follows vector-table order. CPUINT also supports round-robin scheduling.

Important:

- A Level 1 interrupt may preempt a Level 0 ISR.
- Level 0 ISRs do not normally preempt one another.
- Interrupt nesting is not enabled merely by declaring multiple ISRs.
- Global interrupts must be enabled using `sei()`.
- A peripheral interrupt requires both its local interrupt enable and global interrupt enable.
- Pending interrupt flags may cause immediate ISR execution after interrupts are enabled.
- Enabling round-robin scheduling changes Level 0 arbitration behavior.

Use the exact device-specific interrupt vector names from `<avr/io.h>`.

Never invent an interrupt vector name based on a peripheral register name.

## 8. ISR design rules

Interrupt handlers should be short, deterministic, and bounded.

Recommended:

- Acknowledge interrupt flags according to peripheral requirements.
- Capture input state or timestamps.
- Update small counters or flags.
- Defer complex operations to the main loop.
- Keep worst-case execution time below the required interrupt service budget.

Avoid inside ISRs:

- Blocking delays.
- Dynamic allocation.
- Formatted output.
- Long loops.
- Blocking serial transmission.
- Unbounded waiting for peripherals.
- Complex state transitions unrelated to immediate hardware requirements.

Avoid enabling nested interrupts unless explicitly necessary and fully analyzed.

Remember that ISR entry, register preservation, and return consume CPU cycles.

Check actual generated code when ISR latency is critical.

## 9. Atomic shared-state access

On 8-bit AVR devices, multi-byte accesses are not inherently atomic.

Rules:

- Declare state modified by both ISR and main code as `volatile`.
- Protect shared multi-byte reads and writes using `ATOMIC_BLOCK`.
- Prefer `ATOMIC_RESTORESTATE` over unconditional re-enabling.
- Keep atomic regions extremely short.
- Do not assume `volatile` provides atomicity or synchronization.
- Protect read-modify-write operations when both contexts can update the same state.
- Single-byte loads/stores are generally atomic, but compound operations may not be.

Example:

```c
uint32_t now;

ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    now = ticks_ms;
}
```

For elapsed-time comparisons, use unsigned subtraction:

```c
if ((uint32_t)(now - previous) >= interval_ms) {
    previous = now;
}
```

This handles timer wraparound when the interval and observation assumptions are satisfied. Ensure deadlines are checked frequently enough to avoid ambiguity across a full counter wrap.

## 10. Timer-driven state machines

Prefer timer interrupts that maintain a timebase, with application transitions evaluated in the main loop.

Example: output ON for 250 ms, OFF for 5000 ms.

```c
typedef enum {
    STATE_OFF,
    STATE_ON
} output_state_t;

static output_state_t state = STATE_OFF;
static uint32_t state_started;

void state_machine_init(void)
{
    state = STATE_OFF;
    state_started = millis_get();

    output_off();
}

void state_machine_update(void)
{
    uint32_t now = millis_get();

    switch (state) {
    case STATE_OFF:
        if ((uint32_t)(now - state_started) >= 5000) {
            output_on();
            state = STATE_ON;
            state_started = now;
        }
        break;

    case STATE_ON:
        if ((uint32_t)(now - state_started) >= 250) {
            output_off();
            state = STATE_OFF;
            state_started = now;
        }
        break;
    }
}
```

Here `output_on()` and `output_off()` are application-provided GPIO functions.

Call `state_machine_update()` repeatedly from the main loop.

This pattern avoids blocking delays and keeps application logic outside interrupt context.

Transition timing depends on main-loop responsiveness. If edges require hardware-level precision, use timer compare events or hardware waveform generation instead.

For periodic scheduling, distinguish between:

- `previous = now`: prevents immediate catch-up but introduces scheduling drift from loop latency.
- `previous += interval`: preserves the nominal cadence but requires an explicit policy for missed deadlines.

Select the behavior deliberately.

## 11. Hardware and concurrency verification

Before accepting timer-related firmware:

- [ ] Confirm target MCU and peripheral instance.
- [ ] Confirm timer instance and interrupt vector exist.
- [ ] Verify actual CPU/peripheral clock frequency.
- [ ] Recalculate period, frequency, and PWM duty cycle.
- [ ] Check counter width, register bounds, and integer overflow.
- [ ] Verify clock prescaler and waveform mode.
- [ ] Confirm interrupt enable and flag-clear semantics.
- [ ] Check all ISR-shared state for atomicity.
- [ ] Check that ISRs have bounded execution time.
- [ ] Check stack usage, including nested interrupts where applicable.
- [ ] Verify timer output pin and PORTMUX assignments.
- [ ] Check RTC synchronization where applicable.
- [ ] Review applicable silicon errata.
- [ ] Compile with warnings enabled.
- [ ] Measure real timing with an oscilloscope or logic analyzer.

For high-frequency interrupts, calculate CPU utilization:

    utilization = ISR_cycles * interrupt_frequency
                  / CPU_frequency

Include interrupt entry/exit overhead, and evaluate worst-case rather than average execution time.

## 12. Agent review requirements

When implementing or reviewing timer-related code:

1. Identify the exact MCU and verify peripheral availability.
2. Verify register and interrupt-vector definitions against installed AVR-LibC headers.
3. Determine the effective timer clock from the actual clock configuration.
4. Calculate exact periods, register values, and rounding errors.
5. Confirm counting modes, flag behavior, and initialization order.
6. Review interrupt priorities, latency, atomicity, and ISR execution cost.
7. Check hardware pin routing and timer resource conflicts.
8. Compile and inspect warnings; examine disassembly when timing is critical.
9. State what was verified through compilation versus physical hardware testing.

Never claim precise timing solely because register calculations are correct. Oscillator accuracy, interrupt latency, and application scheduling can affect observed behavior.

## References

- Microchip: tinyAVR 1-Series device datasheets, especially TCA, TCB, TCD, RTC, and CPUINT chapters.
- Microchip AN1982: Interrupt System in tinyAVR 0- and 1-series and megaAVR 0-series.
- AVR-LibC: `<avr/interrupt.h>`, `<util/atomic.h>`, and device-specific headers.

