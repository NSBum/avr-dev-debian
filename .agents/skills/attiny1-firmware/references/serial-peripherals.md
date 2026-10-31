# Serial Communication and Peripherals — tinyAVR 1-Series

## Scope

Guidance for implementing USART, SPI, and I2C communication on Microchip tinyAVR 1-series devices using AVR-GCC and AVR-LibC.

Always verify peripheral instances, register definitions, interrupt vectors, and pin routing against the exact target MCU's datasheet and installed device header.

## Terminology: I2C versus TWI

Microchip calls its I2C-compatible hardware peripheral **TWI (Two-Wire Interface)**.

- Use **I2C** in communication with the user, documentation, and explanations.
- Use `TWI` when referring to hardware registers, peripheral instances, datasheet sections, and interrupt vectors.
- Treat user references to "I2C" as references to the TWI peripheral unless context indicates otherwise.
- Do not unnecessarily correct the user's terminology.

Example: "Configure the I2C controller using the `TWI0` peripheral."

## USART

### Initialization

1. Verify the USART instance exists (`USART0`, `USART1`, etc.).
2. Determine the actual peripheral clock frequency.
3. Configure TX/RX pin directions and PORTMUX routing.
4. Set `BAUD` using the tinyAVR fractional baud-rate formula.
5. Configure character size, parity, stop bits, and operating mode.
6. Enable the transmitter and/or receiver through `CTRLB`.
7. Enable receive interrupts if required.
8. Enable global interrupts after initialization when using ISRs.

Do not assume that USART pins or interrupt vectors are identical across devices.

### Baud-rate calculation

**Do not use the traditional AVR `UBRR = F_CPU / (16 * baud) - 1` formula.**

The tinyAVR 1-series USART uses a fractional baud-rate generator.

For asynchronous operation:

```text
USART.BAUD = 64 * f_CLK_PER / (S * baud_rate)
```

Where:

- `f_CLK_PER` is the peripheral clock frequency in Hz.
- `S = 16` for normal asynchronous mode.
- `S = 8` for double-speed asynchronous mode (`CLK2X`).
- `baud_rate` is the desired bits per second.

For normal asynchronous mode:

```c
#define USART_BAUD(baud) \
    ((uint16_t)((4ULL * F_CPU + (baud) / 2) / (baud)))
```

This macro assumes `F_CPU == f_CLK_PER`, uses 16 samples per bit, and rounds to the nearest integer.

Example:

```c
#define F_CPU 20000000UL

USART0.BAUD = USART_BAUD(9600);
```

At 20 MHz and 9600 baud, the register value is approximately 8333.

Important:

- `F_CPU` must represent the actual CPU/peripheral clock, not merely the oscillator frequency.
- Account for clock prescalers and runtime clock changes.
- Use the correct formula for the selected USART operating mode.
- Check the resulting baud error, especially at high baud rates or low clock frequencies.
- Validate register limits and the maximum supported baud rate.
- Ensure the clock source is sufficiently accurate for reliable asynchronous communication.

### Transmit and receive flags

Important USART status flags:

| Flag | Meaning |
|---|---|
| `RXCIF` | Receive data available |
| `DREIF` | Transmit data register empty |
| `TXCIF` | Transmission completely finished |
| `FERR` | Framing error |
| `PERR` | Parity error |
| `BUFOVF` | Hardware receive buffer overflow |

Use:

- `DREIF` before writing another byte to `TXDATAL`.
- `TXCIF` when the final transmitted bit must have left the peripheral.
- `RXCIF` before reading received data.

Do not confuse `DREIF` with `TXCIF`.

### Receive error handling

USART receive error flags are associated with individual received characters.

For normal 8-bit operation, read `RXDATAH` before `RXDATAL` to capture the error flags associated with that character.

```c
uint8_t status = USART0.RXDATAH;
uint8_t data   = USART0.RXDATAL;

if (status & USART_FERR_bm) {
    // Framing error
}

if (status & USART_PERR_bm) {
    // Parity error
}

if (status & USART_BUFOVF_bm) {
    // Hardware receive overflow
}
```

Check the datasheet for exceptions involving 9-bit character modes.

Do not read `RXDATAL` first and then assume the flags in `RXDATAH` still correspond to that byte.

### Interrupt-driven reception

Prefer interrupt-driven reception when incoming bytes can arrive asynchronously.

Typical design:

- USART RX interrupt receives each character.
- ISR places valid bytes into a fixed-size ring buffer.
- Application code consumes bytes outside the ISR.
- ISR records framing, parity, and overflow errors.
- Buffer-full conditions are detected explicitly.

Example receive ISR:

```c
#define RX_BUFFER_SIZE 64

static volatile uint8_t rx_buffer[RX_BUFFER_SIZE];
static volatile uint8_t rx_head;
static volatile uint8_t rx_tail;
static volatile uint8_t rx_overflow;

ISR(USART0_RXC_vect)
{
    uint8_t status = USART0.RXDATAH;
    uint8_t data = USART0.RXDATAL;

    if (status & (USART_FERR_bm |
                  USART_PERR_bm |
                  USART_BUFOVF_bm)) {
        rx_overflow = 1; // Replace with distinct error flags
        return;
    }

    uint8_t next = (rx_head + 1) & (RX_BUFFER_SIZE - 1);

    if (next == rx_tail) {
        rx_overflow = 1;
        return;
    }

    rx_buffer[rx_head] = data;
    rx_head = next;
}
```

This example uses a power-of-two buffer size and deliberately leaves one slot unused.

Requirements:

- Keep ISRs short and deterministic.
- Never perform blocking waits inside an ISR.
- Do not perform lengthy protocol parsing inside the RX ISR.
- Use separate error indicators for hardware errors and software ring-buffer overflow in production code.
- Use `volatile` for ISR-shared state.
- Ensure all shared accesses are atomic or protected appropriately.
- The single-producer/single-consumer ring-buffer pattern can use 8-bit indices without disabling interrupts when ownership and access ordering are correct.
- For larger indices or compound shared-state operations, use `ATOMIC_BLOCK` where necessary.
- Confirm receive processing can sustain the worst-case incoming byte rate.

### Transmit buffering

For nonblocking transmission:

- Use a separate TX ring buffer.
- Enable the Data Register Empty interrupt when data becomes available.
- In the DRE ISR, transfer the next byte to `TXDATAL`.
- Disable the DRE interrupt when the software buffer becomes empty.

Avoid leaving the DRE interrupt permanently enabled when no data is pending.

Use TX Complete detection when transmission must finish before changing hardware state, such as disabling an RS-485 driver.

## SPI

The SPI peripheral supports synchronous serial communication.

Before implementation, determine:

- Controller or peripheral role.
- SPI mode (CPOL/CPHA, modes 0–3).
- Clock frequency.
- Bit order.
- Chip-select behavior.
- Electrical voltage compatibility.

### Controller operation

Typical sequence:

1. Configure MOSI, MISO, SCK, and chip-select pins.
2. Configure SPI mode, clock rate, and bit order.
3. Enable the SPI peripheral.
4. Assert the target device's chip select.
5. Start the transfer by writing the SPI data register.
6. Wait for completion or handle the transfer interrupt.
7. Read the received data and check relevant status flags.
8. Deassert chip select when the transaction is finished.

SPI is full duplex: transmitting a byte also receives a byte.

Reading from an SPI peripheral generally requires transmitting a dummy byte.

### SPI correctness

- Verify the exact device's register and flag definitions.
- Do not reuse classic AVR `SPCR`, `SPSR`, and `SPDR` examples without adaptation.
- Distinguish transfer completion from transmit-data availability where applicable.
- Check write-collision and receive-overflow conditions.
- Observe chip-select setup, hold, and inter-transfer timing.
- Respect the peripheral device's maximum SPI clock frequency.
- Never assume every SPI transaction is exactly one byte.
- Use timeouts for polled transfers.

## I2C (TWI)

The tinyAVR 1-series TWI peripheral implements I2C-compatible communication.

Use "I2C" in user-facing explanations and `TWI0` for hardware register references.

### Electrical requirements

- SDA and SCL require appropriate pull-up resistors.
- Pull-ups must connect to a compatible logic voltage.
- Choose resistance based on bus capacitance, voltage, and required rise time.
- Avoid relying on weak internal pull-ups for general-purpose I2C buses.
- Verify that all devices support the intended bus speed.
- Account for clock stretching where supported.

### Controller transactions

Typical I2C write transaction:

```text
START
Address + Write
ACK
Data byte(s)
ACK after each byte
STOP
```

Typical register read:

```text
START
Address + Write
ACK
Register address
ACK
REPEATED START
Address + Read
ACK
Data byte(s)
NACK final byte
STOP
```

Do not assume every device uses this register-access pattern. Follow the target device's protocol.

### Important TWI conditions

Handle:

- Address acknowledgment and NACK.
- Data acknowledgment and NACK.
- Bus error.
- Arbitration lost.
- Clock stretching.
- Bus-busy conditions.
- Transaction timeout.

On supported tinyAVR TWI controller hardware, relevant status bits include:

- `WIF` — Write interrupt flag.
- `RIF` — Read interrupt flag.
- `RXACK` — Received acknowledgment status; set indicates NACK.
- `ARBLOST` — Arbitration lost.
- `BUSERR` — Bus error.
- `BUSSTATE` — Current bus state.

Use the exact register definitions from the target device header.

### I2C recovery

When a transaction fails:

1. Record the failure reason.
2. Stop the current transaction when the bus state permits.
3. Restore the controller to a known state.
4. Retry only when appropriate and within a bounded retry policy.
5. Report persistent failures to the application.

If SDA remains held low, recovery may require manually clocking SCL before reinitializing the peripheral, provided the hardware permits it.

Never assume that issuing STOP automatically recovers every stuck-bus condition.

## Timeouts

All blocking communication operations should have bounded timeouts.

This applies to:

- USART transmit and receive waits.
- SPI transfer completion.
- I2C address and data phases.
- Bus acquisition.
- Protocol response waits.

Example:

```c
bool usart_send(uint8_t data, uint32_t timeout_ms)
{
    uint32_t deadline = millis() + timeout_ms;

    while (!(USART0.STATUS & USART_DREIF_bm)) {
        if ((int32_t)(millis() - deadline) >= 0) {
            return false;
        }
    }

    USART0.TXDATAL = data;
    return true;
}
```

This assumes an implemented, advancing `millis()` function, bounded timeout values, and a suitable unsigned 32-bit timebase.

Timeout design:

- Prefer monotonic timer-based deadlines.
- Account for timer-counter wraparound.
- Never assume a wait loop executes in a fixed amount of time.
- Ensure the timeout clock continues running during the wait.
- Distinguish per-byte, inter-byte, and whole-transaction timeouts.
- Avoid unbounded retries.
- Return explicit timeout or error status to callers.
- For interrupt-driven protocols, prefer nonblocking state machines when possible.

## Protocol framing

USART transports bytes, not complete application messages.

When implementing serial protocols:

- Define message boundaries explicitly.
- Handle partial messages.
- Handle malformed or truncated frames.
- Enforce maximum frame lengths.
- Validate checksums or CRCs where specified.
- Resynchronize after invalid data.
- Avoid assuming one receive interrupt equals one message.
- Use inter-byte timeouts where the protocol requires them.

For binary protocols, avoid relying on null terminators or newline delimiters unless explicitly defined by the protocol.

## Device-family differences

Do not assume all tinyAVR 1-series MCUs expose identical serial peripherals.

Before generating code, inspect:

- USART instance count.
- SPI and TWI availability.
- Interrupt vector names.
- Alternate pin routing and PORTMUX options.
- Package pin availability.
- Peripheral-specific operating modes.
- Relevant silicon errata.

Check the device-specific header under:

```text
/usr/lib/avr/include/avr/
```

The actual installation path may differ.

Use the device's datasheet and header as authoritative sources.

## Code-review checklist

Before accepting serial communication code, verify:

- [ ] Correct MCU and peripheral instance.
- [ ] Correct pin directions and PORTMUX configuration.
- [ ] Correct clock frequency and USART baud formula.
- [ ] Baud-rate error within acceptable limits.
- [ ] Correct USART frame format.
- [ ] Correct status-flag handling and register access order.
- [ ] Receive-buffer overflow detected.
- [ ] ISR-shared variables handled safely.
- [ ] Interrupts enabled and disabled correctly.
- [ ] No unbounded blocking waits.
- [ ] SPI mode, clock, and chip select verified.
- [ ] I2C pull-ups and electrical levels verified.
- [ ] I2C NACK, bus errors, and arbitration loss handled.
- [ ] Partial and malformed protocol frames handled.
- [ ] Recovery behavior defined for communication failures.
- [ ] Relevant hardware behavior tested on the target MCU.

## References

- Microchip ATtiny1614/1616/1617 datasheet — USART, SPI, TWI, PORTMUX.
- Microchip TB3216 — Getting Started with USART.
- Device-specific AVR-LibC headers for registers, flags, and interrupt vectors.

When datasheet details and example code disagree, prefer the documentation for the exact device and silicon revision.
