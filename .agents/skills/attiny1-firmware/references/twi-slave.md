# tinyAVR 1-series TWI/I²C client (slave) guidance

Use this reference whenever a tinyAVR 1-series device must act as an I²C
client/slave, especially when it exposes registers to a separate controller.
The exact device datasheet and installed header remain authoritative.

## Peripheral model

Microchip names the peripheral TWI and calls the slave side the client in some
documentation. Do not port `TinyWireS` or other older ATtiny USI libraries
directly: those libraries implement a different peripheral, interrupt model,
and pin configuration. They are useful only as conceptual state-machine
references.

For ATtiny3216/3217 with the default route:

- PB1 is TWI0 SDA.
- PB0 is TWI0 SCL.
- `PORTMUX.CTRLB.TWI0` must select the default route (`0`).
- External pull-ups and a shared ground are required.
- `SADDR` stores the 7-bit address in bits 7:1, so configure it as
  `(address << 1)`.

## Initialization checklist

Configure the pins and address before enabling the client:

```c
PORTMUX.CTRLB &= (uint8_t)~PORTMUX_TWI0_bm;
PORTB.DIRCLR = PIN1_bm | PIN0_bm;
TWI0.SADDR = (uint8_t)(slave_address << 1U);
TWI0.SADDRMASK = 0U;
TWI0.SCTRLA = TWI_APIEN_bm | TWI_DIEN_bm | TWI_PIEN_bm |
              TWI_ENABLE_bm;
```

Use external pull-ups; do not assume the weak GPIO pull-ups are adequate for
the bus. Keep the TWI ISR short because the peripheral holds SCL low while a
software response is pending.

## Interrupt state machine

`TWI0_TWIS_vect` combines address/STOP and data events:

- `APIF` with `AP=1`: a matching address was received. Record the direction,
  reset per-transaction transmit state, and send an ACK response.
- `DIF` with `DIR=0`: read `SDATA` to consume the master’s register pointer,
  then ACK. The register pointer must survive a repeated START.
- `DIF` with `DIR=1`: provide the next byte in `SDATA`, then ACK/response.
- `APIF` with `AP=0`: STOP; reset transaction state and complete the
  transaction.

When `APIF` and `DIF` can be pending together, service `DIF` before `APIF` so
the first response byte is loaded before releasing the clock. Use explicit
ACK values in the response command:

```c
TWI0.SCTRLB = TWI_ACKACT_ACK_gc | TWI_SCMD_RESPONSE_gc;
```

## First-byte rule

Do not test `TWI_RXACK_bm` before transmitting the first data byte. `RXACK`
describes the master’s response to a byte that has already been transmitted;
it is not a valid gate for the first byte. Maintain a `tx_started` flag:

```c
if (tx_started == 0U) {
    TWI0.SDATA = register_value(register_pointer);
    tx_started = 1U;
    TWI0.SCTRLB = TWI_ACKACT_ACK_gc | TWI_SCMD_RESPONSE_gc;
} else if ((status & TWI_RXACK_bm) == 0U) {
    ++register_pointer;
    TWI0.SDATA = register_value(register_pointer);
    TWI0.SCTRLB = TWI_ACKACT_ACK_gc | TWI_SCMD_RESPONSE_gc;
} else {
    TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
}
```

This failure mode presents on a logic analyzer as a correct address ACK
followed by `0xFF`: the client acknowledged the address but never drove the
first data byte, so the bus pull-up supplied all ones.

## Master interoperability

For the validated ATtiny3217 polling master sequence, prime a data transfer
before loading the master data register:

```c
TWI0.MCTRLB = TWI_MCMD_RECVTRANS_gc;
TWI0.MDATA = register_number;
```

Then issue the repeated read address and terminate a one-byte read with NACK
plus STOP. A typical register read is:

```text
START, address+W, register pointer, REPEATED START,
address+R, data, NACK, STOP
```

## Debugging procedure

Start with a minimal client that has no UART, timer, parser, or register map:

1. Respond at a fixed known address, such as `0x30`.
2. Return a fixed byte such as `0xA5`.
3. Use a GPIO diagnostic output to show address/STOP ISR activity.
4. Confirm the analyzer sees the fixed byte before restoring application
   registers and cross-peripheral state.

Interpret analyzer results carefully:

- Address NACK: wiring, address, power, pull-up, or client initialization.
- Address ACK followed by `0xFF`: first-byte transmit handling is broken or
  the flashed client is not the device on the observed bus.
- Fixed diagnostic byte works but application register reads fail: the
  register-pointer or application snapshot logic is broken.
