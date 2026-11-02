# Hardware contract

- Target: Microchip ATtiny3217, 24-pin package.
- Clock: internal OSC20M, nominal 20 MHz. `FUSE.OSCCFG` must select 20 MHz;
  firmware disables the main clock prescaler. No clock fuse is changed.
- TWI0 default route: PB1/SDA and PB0/SCL.
- Slave address: 7-bit `0x30`.
- I2C response: fixed byte `0xA5`.
- Diagnostic output: PA4 high while addressed and low after STOP.
- Programming: Atmel-ICE using UPDI, AVRDUDE programmer `atmelice_updi`,
  target part `t3217`. PA0/UPDI remains reserved.

Use external pull-ups on SDA and SCL, shared ground, and compatible bus
voltage levels. This project intentionally has no USART, radar parser, timer,
register pointer, or application logic.
