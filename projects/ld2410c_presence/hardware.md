# Hardware contract

- Target: Microchip ATtiny3217, 24-pin package.
- Clock: internal OSC20M, nominal 20 MHz. `FUSE.OSCCFG` must select 20 MHz;
  firmware disables the main clock prescaler. No clock fuse is changed.
- USART0 default route: PB2/TXD and PB3/RXD. The firmware uses PB3/RXD;
  PB2/TXD is left unused.
- I2C/TWI0 default route: PB1/SDA and PB0/SCL. Provide external pull-up
  resistors to the I2C bus voltage; the firmware does not enable weak GPIO
  pull-ups.
- I2C address selection: PC4 is address bit 0 and PC5 is address bit 1.
  Use external pulldowns for the low state. The resulting 7-bit address is
  `0x30 + (PC4 ? 1 : 0) + (PC5 ? 2 : 0)`, giving `0x30` through `0x33`.
- LD2410C wiring: radar `UART_Tx` -> MCU PB3/RXD; radar GND -> MCU GND;
  radar VCC -> a suitable 5 V supply. Radar UART logic is 3.3 V; confirm
  ATtiny input thresholds and board voltage before physical connection.
- Serial format: 256000 baud, 8 data bits, no parity, 1 stop bit (8-N-1).
- Programming: Atmel-ICE using UPDI, AVRDUDE programmer `atmelice_updi`,
  target part `t3217`. PA0/UPDI remains reserved.
- DFPlayer: PA5 is software-UART TX to DFPlayer RX, PA6 is software-UART
  RX from DFPlayer TX, and PA7 is the active-low DFPlayer BUSY input. Use
  appropriate 3.3 V/5 V level translation and connect grounds together.

The application consumes the LD2410C normal or engineering report stream and
does not send configuration commands. Radar reports never trigger DFPlayer
activity; every DFPlayer command is initiated by an I2C write. DFPlayer UART
operations are deferred to the main loop because they are blocking and must
not run from the TWI interrupt.

## I2C register map

The radar registers are read-only and use the latest valid radar report. A
standard register read is performed by writing the register number, then
issuing a repeated-start read. Writes use the same register pointer and may
write consecutive registers.

| Register | Meaning |
|---:|---|
| `0x00` | Status: `0x00` not detected, `0x01` moving, `0x02` stationary |
| `0x01..0x02` | Selected target distance in cm, little-endian |
| `0x03..0x04` | Moving target distance in cm, little-endian |
| `0x05..0x06` | Stationary target distance in cm, little-endian |
| `0x07` | Raw LD2410C target state: `0x00` none, `0x01` moving, `0x02` stationary, `0x03` both |

### DFPlayer command registers

The generic command is staged in `0x10..0x13`, then submitted by writing any
nonzero value to `0x14`. `0x10` is the DFPlayer command byte, `0x11` requests a
DFPlayer feedback frame when nonzero, and `0x12..0x13` are the 16-bit big-endian
DFPlayer parameter. The command is queued if no prior command is pending;
otherwise it is rejected. The I2C transaction receives an ACK before the
command is actually sent.

Convenience commands are available: write a 16-bit big-endian track to
`0x20..0x21`, then a nonzero value to `0x22` for command `0x03`; write folder
and file indexes to `0x23` and `0x24`, then `0x25` for command `0x0F`; write a
volume from 0 through 30 to `0x26` for command `0x06`.

The generic interface permits DFPlayer query/control commands without
convenience wrappers. For example, command `0x42` requests the software
version, and command `0x3F` requests an initial status response on variants
that support it. Consult the specific DFPlayer firmware manual for command
and parameter meanings.

### DFPlayer status and response registers

| Register | Meaning |
|---:|---|
| `0x30` | Status: `0` idle, `1` queued, `2` transmitting/waiting, `3` sent without feedback, `4` response received, `5` error |
| `0x31` | Last transport result: `0` OK, `1` timeout, `2` frame error, `3` transport error, `4` invalid argument |
| `0x32` | Last response command byte |
| `0x33` | Last response feedback byte |
| `0x34..0x35` | Last response parameter, big-endian |
| `0x36` | DFPlayer BUSY state: `1` playing/busy, `0` idle |
| `0x37` | Number of commands rejected because the previous command was pending |

When feedback is requested, the raw response fields are updated after a valid
frame. A DFPlayer ACK normally has response command `0x41`; error and query
responses are exposed without being discarded, so the host can interpret
device-specific playback or query codes.
