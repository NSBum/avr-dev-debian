
# GPIO and PORTMUX — tinyAVR 1-Series

## Scope

Applies to Microchip tinyAVR 1-series devices.

Register availability, pin mappings, peripheral routes, and package
bond-outs vary by MCU. Always verify against the exact device datasheet
and installed AVR-LibC device header.

## PORT versus VPORT

Each GPIO port has two register interfaces:

- `PORTx`: Full GPIO control, including atomic set/clear/toggle
  registers and per-pin configuration.
- `VPORTx`: Compact, low-I/O-address interface for fast access to
  direction, output, input, and interrupt flags.

Common registers:

| PORT | VPORT | Purpose |
|---|---|---|
| `PORTx.DIR` | `VPORTx.DIR` | Direction: 1=output |
| `PORTx.OUT` | `VPORTx.OUT` | Output latch |
| `PORTx.IN` | `VPORTx.IN` | Input pin state |
| `PORTx.INTFLAGS` | `VPORTx.INTFLAGS` | Pin interrupt flags |
| `PORTx.DIRSET` | — | Set direction bits |
| `PORTx.DIRCLR` | — | Clear direction bits |
| `PORTx.DIRTGL` | — | Toggle direction bits |
| `PORTx.OUTSET` | — | Set output bits |
| `PORTx.OUTCLR` | — | Clear output bits |
| `PORTx.OUTTGL` | — | Toggle output bits |
| `PORTx.PINnCTRL` | — | Per-pin configuration |

Prefer PORT set/clear/toggle registers for GPIO modifications.
These avoid read-modify-write operations and preserve unrelated bits.

```c
// Configure PA3 as output, initially low
PORTA.OUTCLR = PIN3_bm;
PORTA.DIRSET = PIN3_bm;

// Drive output
PORTA.OUTSET = PIN3_bm;
PORTA.OUTCLR = PIN3_bm;
PORTA.OUTTGL = PIN3_bm;

// Read input
bool high = (PORTA.IN & PIN2_bm) != 0;
```

VPORT is useful for latency-sensitive operations or compact
single-bit access. It does not provide the full PORT register set.

Do not assume read-modify-write operations on PORT or VPORT
are atomic with respect to interrupts.

## Inputs and pull-ups

GPIO pins default to inputs after reset, subject to special pin
functions and device configuration.

Configure an input explicitly:

```c
PORTA.DIRCLR = PIN2_bm;
```

Enable the internal pull-up using `PINnCTRL`:

```c
PORTA.PIN2CTRL |= PORT_PULLUPEN_bm;
```

Disable it:

```c
PORTA.PIN2CTRL &= ~PORT_PULLUPEN_bm;
```

Important:

- Pull-ups are configured per pin, not through `DIR` or `OUT`.
- An input without a pull-up or external bias may float.
- Internal pull-ups are relatively weak and unsuitable for
  applications requiring a precise resistance.
- For active-low switches, a pull-up typically makes the idle
  state high and the asserted state low.
- Do not enable pull-ups indiscriminately on analog inputs.

Example active-low input:

```c
PORTA.DIRCLR = PIN2_bm;
PORTA.PIN2CTRL = PORT_PULLUPEN_bm;

bool pressed = !(PORTA.IN & PIN2_bm);
```

When changing `PINnCTRL`, preserve existing configuration bits
unless intentionally replacing the entire register.

## Input sensing and interrupts

Input sensing is controlled by the `ISC` field of `PINnCTRL`.

Common configurations:

- `PORT_ISC_INTDISABLE_gc`: Input buffer enabled,
  pin interrupt disabled.
- `PORT_ISC_BOTHEDGES_gc`: Interrupt on either edge.
- `PORT_ISC_RISING_gc`: Interrupt on rising edge.
- `PORT_ISC_FALLING_gc`: Interrupt on falling edge.
- `PORT_ISC_INPUT_DISABLE_gc`: Disable digital input buffer.
- `PORT_ISC_LEVEL_gc`: Low-level sensing.

Example falling-edge interrupt:

```c
PORTA.DIRCLR = PIN2_bm;

PORTA.PIN2CTRL =
    PORT_PULLUPEN_bm | PORT_ISC_FALLING_gc;
```

Pin interrupts are handled by port-level interrupt vectors,
not necessarily one vector per pin.

```c
ISR(PORTA_PORT_vect)
{
    uint8_t flags = PORTA.INTFLAGS;

    // Clear captured flags (write-one-to-clear)
    PORTA.INTFLAGS = flags;

    if (flags & PIN2_bm) {
        // Handle PA2 event
    }
}
```

Rules:

- Clear interrupt flags by writing 1 to the relevant bits.
- Handle multiple pending pin flags if sharing a port ISR.
- Enable global interrupts with `sei()` when required.
- Mechanical switches generally require debounce logic.
- Level-sensitive interrupts can retrigger while the
  asserted level remains present.
- Disable the digital input buffer when appropriate for
  analog-only use, considering peripheral requirements.

## Output drive

Output direction and output latch are independent.

Configure the output latch before enabling output drive to
avoid unintended startup transitions.

```c
PORTA.OUTCLR = PIN3_bm;
PORTA.DIRSET = PIN3_bm;
```

GPIO outputs are push-pull by default.

Do not assume a GPIO can directly drive:

- Relays
- Motors
- High-current LEDs
- Loads operating at incompatible voltages

Check per-pin current limits, total port/device limits,
output voltage levels, and external driver requirements
in the device datasheet.

For open-drain-like GPIO operation, use an external pull-up
and switch between driving low and high impedance:

```c
// Assert low
PORTA.OUTCLR = PIN3_bm;
PORTA.DIRSET = PIN3_bm;

// Release line
PORTA.DIRCLR = PIN3_bm;
```

This is software-emulated open drain, not a dedicated
open-drain output mode. Account for timing and contention.

## Peripheral routing — PORTMUX

Many peripheral outputs and inputs share GPIO pins.

`PORTMUX` controls supported alternate pin routes for
peripherals such as USART, SPI, TWI, and timers.

Before enabling a peripheral:

1. Identify its required signals.
2. Consult the exact device's pin multiplexing table.
3. Select the intended PORTMUX route.
4. Verify that every required pin exists on the package.
5. Ensure the pins are not allocated to another function.
6. Configure peripheral operation and required GPIO settings.

Do not assume that PORTMUX:

- Provides arbitrary pin assignment.
- Can route any peripheral to any GPIO.
- Has identical registers across tinyAVR devices.
- Automatically resolves conflicts between peripherals.

A PORTMUX setting selects among fixed hardware routes.

Use the exact device header's register names and named
bit masks or enumerations. Do not invent PORTMUX symbols
or copy settings from a different AVR family.

For peripheral-controlled pins, follow the peripheral
datasheet requirements for GPIO direction, pull-ups,
and input-buffer configuration.

## Special pins and conflicts

### UPDI / RESET

On many tinyAVR 1-series devices, PA0 is shared with UPDI
and/or RESET functionality.

Treat the UPDI pin as reserved during development.

Do not repurpose it for general GPIO unless:

- The exact device supports the configuration.
- The relevant fuse behavior is understood.
- A reliable programming recovery procedure is available.

Changing the UPDI/RESET pin configuration can prevent
ordinary reprogramming and may require high-voltage UPDI
programming for recovery.

### Other pin functions

Check for conflicts involving:

- External clock inputs and clock outputs.
- Crystal oscillator pins, where applicable.
- ADC, DAC, and analog comparator connections.
- USART, SPI, and TWI signals.
- Timer waveform outputs.
- Event system outputs.
- Configurable Custom Logic (CCL).
- Board-specific external circuitry.

A pin may support several alternate functions, but
these functions cannot necessarily operate concurrently.

Some peripheral routes affect groups of pins rather than
individual signals.

Distinguish the MCU's logical pin name (for example PA3)
from the physical package pin number.

## Pin allocation checklist

Before writing GPIO or peripheral initialization code:

- [ ] Exact MCU and package identified.
- [ ] Every logical pin exists on the selected package.
- [ ] Pin functions checked against the datasheet.
- [ ] UPDI and other programming pins protected.
- [ ] PORTMUX routing checked for shared-pin conflicts.
- [ ] Input pull-ups and sensing modes explicitly defined.
- [ ] Output startup levels defined before enabling drive.
- [ ] External voltage and current requirements checked.
- [ ] Register names and bit masks verified against
      the installed AVR-LibC device header.

If a mapping or register definition is uncertain, inspect
the device header and datasheet rather than guessing.

