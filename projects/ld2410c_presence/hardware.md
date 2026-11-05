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

An I2C write has this form:

```text
START, slave-address + write, register, value[, value...], STOP
```

After the first byte, each value is written to the selected register and the
register pointer increments. Thus, to write registers `0x23`, `0x24`, and
`0x25`, send one transaction beginning with `0x23` followed by three values;
do not send the register address before every value.

The generic command interface uses registers `0x10..0x14`:

| Register | Write value |
|---:|---|
| `0x10` | Native DFPlayer command byte |
| `0x11` | `0` for no response requested, nonzero to request feedback |
| `0x12` | Command parameter high byte |
| `0x13` | Command parameter low byte |
| `0x14` | Write nonzero to submit the staged command |

The parameter is big-endian. Writing `0x14` queues the command for the main
loop; the I2C write being acknowledged does not mean that the DFPlayer UART
frame has already been sent. There is one deferred command slot. If that slot
is occupied, a new command is rejected and `0x37` increments.

#### Example: play a file from a folder

To play file `7` in folder `2`, use the convenience registers and send:

```text
I2C write to address 0x30: 23 02 07 01
                           ^^    ^^
                           |     +-- write 1 to 0x25: play folder/file
                           +-------- register pointer 0x23, then:
                                     0x23 = folder 2
                                     0x24 = file 7
```

The equivalent Linux `i2c-tools` transaction is:

```sh
i2ctransfer -y 1 w4@0x30 0x23 0x02 0x07 0x01
```

This sends native DFPlayer command `0x0F` with parameter `0x0207`. Folder and
file values are one byte each. The DFPlayer's file naming and folder rules
still apply; this register interface does not rename or index files for it.

#### Example: play a track by global index

To play track `5`, send `0x0005` big-endian to `0x20..0x21`, then write a
nonzero value to `0x22`:

```text
I2C write to address 0x30: 20 00 05 01
i2ctransfer -y 1 w4@0x30 0x20 0x00 0x05 0x01
```

This sends native DFPlayer command `0x03` with parameter `0x0005`.

#### Example: set volume

Volume is limited to `0..30`. To set volume to `24`:

```text
I2C write to address 0x30: 26 18
i2ctransfer -y 1 w2@0x30 0x26 0x18
```

This queues native command `0x06` with parameter `24`. A value above `30` is
rejected locally and does not reach the DFPlayer.

#### Example: play a folder/file and request feedback

The convenience folder/file register does not request feedback. Use the
generic interface when the host must inspect the DFPlayer response. To play
folder `2`, file `7`, with feedback enabled, send:

```text
I2C write to address 0x30: 10 0F 01 02 07 01
                           ^^ command 0x0F
                              ^^ feedback requested
                                 ^^ ^^ parameter 0x0207
                                             ^^ execute
i2ctransfer -y 1 w6@0x30 0x10 0x0F 0x01 0x02 0x07 0x01
```

The response is read from registers `0x32..0x35` after register `0x30`
reports status `4` (response received). A host should not treat the initial
I2C ACK as playback success; it should poll the status/result registers and,
when applicable, interpret the returned DFPlayer response command.

The generic interface permits DFPlayer query/control commands without
convenience wrappers. For example, command `0x42` requests the software
version, and command `0x3F` requests an initial status response on variants
that support it. Consult the specific DFPlayer firmware manual for command
and parameter meanings. For example, to request command `0x42` with feedback
and parameter `0`, send `10 42 01 00 00 01` beginning at register `0x10`.

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

To read one or more registers, write the starting register and issue a
repeated-start read. For example, to read the complete DFPlayer result and
response block:

```text
I2C write:  30
I2C read:   six bytes -> status, result, response-command,
                       response-feedback, response-parameter-high,
                       response-parameter-low
```

With Linux `i2c-tools`:

```sh
i2ctransfer -y 1 w1@0x30 0x30 r6
```

The result codes at `0x31` are `0 = OK`, `1 = timeout`, `2 = frame error`,
`3 = transport error`, and `4 = invalid argument`. These describe the local
UART/response transaction. The raw DFPlayer response at `0x32..0x35` carries
the device-specific ACK, error, or query result; a normal DFPlayer ACK uses
response command `0x41`.
