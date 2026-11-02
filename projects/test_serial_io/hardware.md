# Hardware contract

- Target: Microchip ATtiny3217, 24-pin VQFN package.
- Clock: internal OSC20M, nominal 20 MHz. `FUSE.OSCCFG` must select 20 MHz;
  firmware disables the main clock prescaler. No clock fuse is changed.
- GPIO: PA4, physical pin 5, initialized low and controlled by the command
  processor.
- USART0 default route: PB2/TXD, physical pin 14, and PB3/RXD, physical pin
  13. No PORTMUX alternate route is selected.
- FT232 wiring: MCU PB2/TXD -> FT232 RXD; MCU PB3/RXD <- FT232 TXD; common
  ground. Use a TTL/CMOS FT232 interface at a voltage compatible with the
  ATtiny3217 supply; do not use an RS-232-level adapter.
- Serial format: 9600 baud, 8 data bits, no parity, 1 stop bit.
- Programming: Atmel-ICE using UPDI, AVRDUDE programmer `atmelice_updi`,
  target part `t3217`. PA0/UPDI remains reserved.

The exact board supply voltage, LED/load connected to PA4, and FT232 logic
voltage remain physical-board checks.
