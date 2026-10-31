
# Clocks and Configuration Change Protection (CCP)

## Scope

This reference describes clock configuration, prescaling, oscillator
selection, Configuration Change Protection (CCP), and clock-dependent
firmware calculations for Microchip tinyAVR 1-series devices.

Always consult the exact target device's datasheet and installed AVR-LibC
device header. Do not assume that clock sources, external oscillator
support, register fields, or operating limits are identical across
devices.

## 1. Clock architecture

The main clock path is:

    Clock source
         |
         v
      CLK_MAIN
         |
         v
    Main prescaler
         |
         v
       CLK_PER
         |
         +---- CPU clock domain
         |
         +---- Synchronous peripherals

The available main clock sources may include:

- OSC20M: Factory-calibrated internal 16/20 MHz oscillator
- OSCULP32K: Internal ultra-low-power 32.768 kHz oscillator
- XOSC32K: External 32.768 kHz crystal oscillator
- EXTCLK: External digital clock input

External clock and crystal support is device-dependent.

Additional clock domains, including those used by RTC, WDT, and certain
timers, may operate independently of the main CPU/peripheral clock.

Do not assume every peripheral uses CLK_PER.

### Clock terminology

CLK_MAIN:
    Clock selected by CLKCTRL.MCLKCTRLA.

CLK_PER:
    Main clock after application of the main prescaler.

CLK_CPU:
    CPU clock derived from the main clock path.

F_CPU:
    Compile-time software declaration of the CPU frequency. It is not
    a hardware register and does not configure the oscillator.

## 2. Clock configuration registers

Important CLKCTRL registers include:

| Register | Purpose |
|---|---|
| MCLKCTRLA | Main clock source selection; CLKOUT |
| MCLKCTRLB | Main clock prescaler selection and enable |
| MCLKLOCK | Clock configuration lock |
| MCLKSTATUS | Oscillator and clock-switching status |
| OSC20MCTRLA | Internal 16/20 MHz oscillator control |
| OSC20MCALIBA | Internal oscillator calibration |
| OSC20MCALIBB | Temperature calibration and calibration lock |
| OSC32KCTRLA | Internal 32 kHz oscillator control |
| XOSC32KCTRLA | External 32.768 kHz oscillator control, if implemented |

Consult the device datasheet before using any of these registers.

## 3. Clock configuration after reset

For representative tinyAVR 1-series devices, including ATtiny1616
and ATtiny3216:

- The main clock source after reset is OSC20M.
- The main clock prescaler is enabled.
- The initial prescaler division factor is 6.
- The oscillator runs at 16 MHz or 20 MHz according to FUSE.OSCCFG.

Therefore:

| Oscillator | Prescaler | CLK_PER |
|---|---|---|
| 16 MHz | 6 | Approximately 2.667 MHz |
| 20 MHz | 6 | Approximately 3.333 MHz |

These values describe reset behaviour, not necessarily the configuration
after application startup code executes.

For another target, confirm the reset defaults in its datasheet.

Do not assume that an internal oscillator configured for 20 MHz means
the CPU executes at 20 MHz.

## 4. Oscillator frequency selection

On representative tinyAVR 1-series devices, the OSC20M oscillator
frequency is selected using the FREQSEL field of FUSE.OSCCFG.

The factory calibration settings corresponding to the selected
frequency are loaded during reset.

Changing the main clock prescaler does not change the oscillator's
selected center frequency.

Likewise, writing CLKCTRL.MCLKCTRLA to select OSC20M does not switch
the oscillator between 16 MHz and 20 MHz.

Fuse configuration is a separate programming operation.

Before writing firmware that depends on a particular frequency:

1. Determine the target MCU.
2. Determine the configured FUSE.OSCCFG value.
3. Determine the oscillator's nominal frequency.
4. Determine the main clock prescaler.
5. Calculate the resulting CPU/peripheral frequency.
6. Configure F_CPU consistently.

Do not modify oscillator calibration registers merely to select
16 MHz versus 20 MHz.

## 5. Main clock prescaler

The main prescaler is controlled by:

    CLKCTRL.MCLKCTRLB

Relevant fields:

    PEN   Prescaler enable
    PDIV  Prescaler division selection

The supported divisors on representative tinyAVR 1-series devices
include:

    2, 4, 6, 8, 10, 16, 24, 32, 48, 64

Disabling the prescaler gives division by 1.

Use the symbolic enumeration values defined by the device header,
rather than encoding the PDIV register field numerically.

### Example: Run at 10 MHz from a 20 MHz oscillator

```c
#include <avr/io.h>

static void clock_init(void)
{
    _PROTECTED_WRITE(
        CLKCTRL.MCLKCTRLB,
        CLKCTRL_PDIV_2X_gc | CLKCTRL_PEN_bm
    );
}
```

This assumes the OSC20M oscillator is configured for 20 MHz.

The resulting frequency is:

    F_CPU = 20 MHz / 2 = 10 MHz

A 16 MHz oscillator with the same prescaler would produce 8 MHz.

### Example: Disable the prescaler

```c
static void clock_init(void)
{
    _PROTECTED_WRITE(
        CLKCTRL.MCLKCTRLB,
        0
    );
}
```

This selects division by 1.

The resulting CPU/peripheral frequency is the selected main clock
frequency.

Before disabling the prescaler, verify the maximum permitted operating
frequency for the supply voltage and temperature range.

Do not assume that 20 MHz operation is permitted at every voltage.

## 6. Configuration Change Protection (CCP)

Certain critical control registers cannot be modified by ordinary
register writes.

The CPU provides Configuration Change Protection (CCP) to prevent
accidental changes to these registers.

A protected write requires:

1. Writing the appropriate key to CPU.CCP.
2. Writing the protected register within the permitted instruction
   window.

For I/O register protection, the key is CCP_IOREG_gc.

The protected write must take place within four CPU instructions
following the key write, as specified by the architecture.

An ordinary write without the unlock sequence is ignored.

### Preferred method

Use AVR-LibC's _PROTECTED_WRITE() macro.

```c
#include <avr/io.h>

_PROTECTED_WRITE(
    CLKCTRL.MCLKCTRLB,
    CLKCTRL_PDIV_2X_gc | CLKCTRL_PEN_bm
);
```

The macro handles the required unlock and protected write sequence.

Do not manually implement CCP timing unless there is a justified
reason and the generated assembly has been inspected.

### Incorrect example

```c
CLKCTRL.MCLKCTRLB = CLKCTRL_PDIV_2X_gc |
                    CLKCTRL_PEN_bm;
```

This is an ordinary write to a CCP-protected register.

It will not change the configuration.

### Avoid manually separating the unlock and write

Do not use code such as:

```c
CPU.CCP = CCP_IOREG_gc;

some_function();

CLKCTRL.MCLKCTRLB = value;
```

The function call and intervening instructions violate the required
timing sequence.

Compiler optimization also makes hand-written C timing assumptions
unsafe.

### CCP-protected register handling

When updating a protected register:

- Use _PROTECTED_WRITE() with the correct register and complete value.
- Perform read/modify/write computations before the protected write.
- Do not insert function calls or other operations into a manually
  implemented unlock sequence.
- Do not assume an ordinary assignment to a protected register works.
- Verify the register's documented protection mechanism.

A protected write must not be confused with an ordinary atomic update
of a shared variable.

### Preserving other bits

When modifying only selected fields in a protected register,
compute the complete new value before issuing the protected write.

```c
uint8_t value = CLKCTRL.MCLKCTRLA;

value |= CLKCTRL_CLKOUT_bm;

_PROTECTED_WRITE(CLKCTRL.MCLKCTRLA, value);
```

This preserves the other writable fields rather than replacing the
entire register value indiscriminately.

Check reserved bits and register-specific write semantics before
using read/modify/write.

## 7. CCP and interrupts

The CCP unlock sequence is timing-critical.

Prefer _PROTECTED_WRITE() instead of a custom C sequence. AVR-LibC
implements the required instruction-level sequence.

If implementing CCP manually in assembly:

- Inspect the exact sequence specified by the device.
- Ensure that no interrupt or unrelated instruction can invalidate
  the unlock/write timing.
- Verify instruction count in the generated machine code.
- Restore interrupt state correctly if interrupt masking is required.

Do not disable interrupts indiscriminately around every protected
write. Use the established library primitive unless a particular
operation requires additional synchronization.

CCP protects critical hardware registers from accidental writes;
it is not a general mutual-exclusion mechanism.

## 8. Clock source switching

The main clock source is selected using:

    CLKCTRL.MCLKCTRLA.CLKSEL

Typical symbolic values include:

    CLKCTRL_CLKSEL_OSC20M_gc
    CLKCTRL_CLKSEL_OSCULP32K_gc
    CLKCTRL_CLKSEL_XOSC32K_gc
    CLKCTRL_CLKSEL_EXTCLK_gc

Not all values are available or usable on every MCU.

Example: Select the internal 16/20 MHz oscillator.

```c
_PROTECTED_WRITE(
    CLKCTRL.MCLKCTRLA,
    CLKCTRL_CLKSEL_OSC20M_gc
);
```

This does not by itself determine whether OSC20M is operating at
16 MHz or 20 MHz.

### Switching to an external source

Before switching to an external clock source:

1. Confirm that the MCU supports that source.
2. Confirm required oscillator or clock input hardware is present.
3. Configure and enable the oscillator as documented.
4. Wait for the relevant readiness/status indication.
5. Select the new source using a CCP-protected write.
6. Check any documented clock-switching status requirements.
7. Recalculate all clock-dependent peripheral settings.

Do not switch to an absent or unstable clock source.

Clock switching may affect CPU execution timing, USART baud rates,
timer periods, and timeouts.

## 9. Clock lock

The main clock configuration can be protected against further changes
using the MCLKLOCK register where supported.

Do not enable clock locking during ordinary initialization unless
the application explicitly requires it.

The lock can prevent subsequent software changes to clock
configuration until the applicable reset condition.

Consult the datasheet for exactly which registers are affected and
how the lock is cleared.

Do not confuse MCLKLOCK with oscillator calibration locking.

## 10. F_CPU and timing calculations

F_CPU must describe the actual CPU clock used by the firmware,
not merely the oscillator frequency.

For example:

```c
#define F_CPU 10000000UL

#include <avr/io.h>
#include <util/delay.h>
```

This declaration is appropriate when the configured hardware
actually runs at 10 MHz.

F_CPU does not alter the clock hardware.

### Delay routines

Functions such as _delay_ms() depend on F_CPU.

Incorrect F_CPU values cause incorrect delays.

For example, when actual CPU speed is 10 MHz but F_CPU is declared
as 20 MHz, a nominal 100 ms software delay may take approximately
200 ms.

Use compile-time constant delay arguments where required by
the AVR-LibC delay implementation.

### USART baud calculations

USART baud-rate settings depend on the USART peripheral clock.

For a tinyAVR 1-series USART operating in asynchronous normal mode,
the documented baud-register relationship is:

    BAUD = 64 * f_CLK_PER / (S * baud_rate)

where S is the samples-per-bit factor for the selected mode.

For normal asynchronous mode:

    S = 16

For double-speed asynchronous mode:

    S = 8

Account for integer rounding, register width, and achievable baud
error.

Confirm the exact USART mode and clock source before calculating BAUD.

A common normal-mode calculation is:

```c
#define USART_BAUD_VALUE(clk_per, baud) \
    ((uint16_t)((4UL * (clk_per) + (baud) / 2UL) / (baud)))
```

This macro is for asynchronous normal mode with S = 16.

Check the resulting baud error and clock accuracy, particularly at
high baud rates.

### Timer calculations

For a timer clocked directly from CLK_PER through a prescaler:

    f_TIMER = f_CLK_PER / timer_prescaler

For a timer with a period register counting from zero:

    period_counts = f_TIMER * desired_period_seconds

    PER = period_counts - 1

The exact formula depends on timer mode and counting behaviour.

Some peripherals use alternative clock sources, event inputs,
or independent prescalers.

Consult the specific peripheral's clock diagram.

## 11. Runtime clock changes

Changing the CPU/peripheral clock after initialization can invalidate
existing peripheral configuration.

Before implementing dynamic clock switching:

1. Identify every clock-dependent peripheral.
2. Determine whether active transactions must be completed.
3. Change the clock source or prescaler safely.
4. Update USART baud settings as necessary.
5. Reconfigure timers and PWM periods as necessary.
6. Recalculate software timing assumptions.
7. Verify peripheral operation after the transition.

F_CPU is a compile-time constant and cannot automatically follow
runtime frequency changes.

For applications with dynamic CPU frequencies, avoid relying on
fixed-frequency delay calculations for critical runtime timing.

Use a suitable timer or independent reference clock instead.

## 12. Oscillator accuracy and calibration

The internal OSC20M oscillator is factory calibrated.

Its actual frequency may vary with:

- Supply voltage
- Temperature
- Factory calibration tolerance
- Additional application calibration

Do not assume that a nominal 20 MHz oscillator generates exactly
20,000,000 Hz.

For applications requiring accurate UART communication, long-term
timekeeping, or precise timing:

- Consult oscillator accuracy specifications.
- Calculate the allowed frequency error.
- Consider the combined transmitter/receiver baud-rate error.
- Consider an appropriate external reference when supported.
- Do not modify factory calibration values without a documented
  calibration procedure.

For manual oscillator calibration, follow the datasheet's precautions
concerning frequency overshoot, voltage limits, and temporary
prescaling.

## 13. Operating frequency and supply voltage

The maximum safe clock frequency depends on the device's operating
voltage and other conditions.

Before selecting full-speed operation:

- Check the General Operating Ratings section.
- Verify the actual minimum supply voltage.
- Consider brown-out behaviour and supply transients.
- Check temperature-dependent restrictions.
- Ensure all peripherals remain within their documented limits.

Never assume that a device supporting 20 MHz can operate at that
frequency throughout its entire voltage range.

## 14. Clock output for debugging

Many tinyAVR 1-series devices provide CLKOUT functionality through:

    CLKCTRL.MCLKCTRLA.CLKOUT

This outputs the system clock on the device's designated CLKOUT pin.

The physical pin must be verified from the exact device's pin
multiplexing documentation.

Example:

```c
uint8_t value = CLKCTRL.MCLKCTRLA;

value |= CLKCTRL_CLKOUT_bm;

_PROTECTED_WRITE(CLKCTRL.MCLKCTRLA, value);
```

CLKOUT may be useful to verify the effective system clock with an
oscilloscope or frequency counter.

Check the documented CLKOUT behaviour, especially in sleep modes,
and confirm which internal clock stage is driven to the pin.

Do not assume CLKOUT is available on an arbitrary GPIO.

## 15. Validation and debugging

When diagnosing clock-related problems:

1. Confirm the exact MCU specified by -mmcu.
2. Inspect FUSE.OSCCFG and its FREQSEL field.
3. Read CLKCTRL.MCLKCTRLA.
4. Read CLKCTRL.MCLKCTRLB.
5. Inspect CLKCTRL.MCLKSTATUS.
6. Calculate the expected CLK_PER frequency.
7. Compare the actual clock against F_CPU.
8. Verify protected writes were performed correctly.
9. Check peripheral-specific clock sources and prescalers.
10. Measure a clock-dependent output when practical.

### Inspect generated assembly

Use:

```sh
avr-objdump -d firmware.elf
```

or:

```sh
avr-objdump -S firmware.elf
```

The -S option interleaves source code when suitable debug
information is available.

For CCP operations, inspect the generated instructions if a
custom implementation is used.

The protected write must occur inside the required instruction
window after the unlock key.

Do not rely solely on C source appearance to determine instruction
timing.

## 16. Common mistakes

### Assuming 20 MHz after reset

Incorrect: Treating the internal oscillator frequency as F_CPU.

Correct: Account for the reset prescaler and oscillator fuse.

### Changing F_CPU without changing hardware

Incorrect: Defining F_CPU as 10000000UL and assuming this changes
the CPU frequency.

Correct: Configure the hardware clock and keep F_CPU consistent.

### Writing a protected register directly

Incorrect: Ordinary assignment to CLKCTRL.MCLKCTRLB.

Correct: Use _PROTECTED_WRITE().

### Assuming 20 MHz oscillator operation

Incorrect: Selecting OSC20M and assuming it runs at 20 MHz.

Correct: Inspect FUSE.OSCCFG.FREQSEL.

### Ignoring clock changes in USART configuration

Incorrect: Changing CLK_PER without updating a baud-rate calculation.

Correct: Calculate baud registers using the actual USART clock.

### Using a main-clock formula for every timer

Incorrect: Assuming all timer clock sources derive identically
from CLK_PER.

Correct: Inspect the individual peripheral's clock architecture.

### Selecting an external clock without validating hardware

Incorrect: Switching to EXTCLK or XOSC32K without ensuring that
the source exists and is usable.

Correct: Confirm hardware, enable the source, verify readiness,
and follow the clock-switching procedure.

## 17. Rules for firmware generation

When generating or modifying tinyAVR 1-series firmware:

1. Identify the exact MCU before writing clock configuration code.
2. Determine the clock source, fuse configuration, and prescaler.
3. Calculate the actual CPU/peripheral frequency.
4. Use _PROTECTED_WRITE() for CCP-protected I/O registers.
5. Use device-header symbolic constants rather than magic values.
6. Never assume F_CPU changes hardware clock configuration.
7. Verify operating-frequency limits against supply voltage.
8. Identify the clock source used by each relevant peripheral.
9. Recalculate peripheral settings when clock assumptions change.
10. Avoid oscillator calibration changes without explicit requirements.
11. Avoid enabling clock locks unless explicitly requested.
12. Use the installed device header and datasheet to resolve uncertainty.
13. Prefer simple, deterministic clock initialization.
14. State clock assumptions clearly in comments and build settings.
15. Validate timing-sensitive configuration against generated assembly
    when necessary.

## References

- Microchip ATtiny1614/1616/1617 Complete Datasheet, DS40002204A
- Microchip ATtiny3216/ATtiny1616 Datasheet, 40001997B
- Microchip ATtiny212/214/412/414/416 Datasheet, DS40002287A
- AVR-LibC: avr/io.h and _PROTECTED_WRITE()
- Microchip documentation: CLKCTRL and CCP peripheral chapters

For authoritative register details, use the datasheet corresponding
to the exact target device.

