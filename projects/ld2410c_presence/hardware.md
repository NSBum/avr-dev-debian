# Hardware contract

- Target: Microchip ATtiny3217, 24-pin package.
- Clock: internal OSC20M, nominal 20 MHz. `FUSE.OSCCFG` must select 20 MHz;
  firmware disables the main clock prescaler. No clock fuse is changed.
- Indicator: PA4, active-high LED or other suitable load.
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

The application consumes the LD2410C normal or engineering report stream and
does not send configuration commands. Target distances are compared in cm;
the 0--1 m boundary is inclusive at 100 cm. If both moving and stationary
targets are reported within range, moving indication takes priority.

## I2C register map

The registers are read-only and use the latest valid radar report. A standard
register read is performed by writing the register number, then issuing a
repeated-start read.

| Register | Meaning |
|---:|---|
| `0x00` | Status: `0x00` not detected, `0x01` moving, `0x02` stationary |
| `0x01..0x02` | Selected target distance in cm, little-endian |
| `0x03..0x04` | Moving target distance in cm, little-endian |
| `0x05..0x06` | Stationary target distance in cm, little-endian |
| `0x07` | Raw LD2410C target state: `0x00` none, `0x01` moving, `0x02` stationary, `0x03` both |
