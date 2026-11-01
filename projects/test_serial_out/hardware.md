# Hardware contract

- Target: Microchip ATtiny3217, 24-pin VQFN package.
- Clock: internal OSC20M, nominal 20 MHz. `FUSE.OSCCFG` must select 20 MHz;
  firmware disables the main clock prescaler. No clock fuse is changed.
- USART route: USART0 default route, PB2/TXD. The datasheet maps PB2 to
  physical pin 14 on the VQFN-24 package. PB3/RXD is unused by this
  transmit-only test.
- FT232 connection: MCU PB2/TXD -> FT232 RXD, MCU ground -> FT232 ground.
  Keep the FT232 I/O voltage compatible with the MCU supply; do not connect
  incompatible logic levels. FT232 TXD -> PA2/RXD is optional and unused.
- Serial format: 9600 baud, 8 data bits, no parity, 1 stop bit.
- Programming: Atmel-ICE using UPDI, AVRDUDE programmer `atmelice_updi`,
  target part `t3217`. PA0/UPDI remains reserved.

The exact FT232 device node and board supply voltage must be confirmed on the
host and physical board.
