# Hardware contract

- Target: Microchip ATtiny3217, 24-pin package.
- Clock: internal OSC20M, nominal 20 MHz. `FUSE.OSCCFG` must select 20 MHz;
  firmware disables the main clock prescaler. No clock fuse is changed.
- I2C/TWI0 master: default PB1/SDA and PB0/SCL. Connect these to the
  corresponding pins on the serial-to-I2C ATtiny3217 project.
- I2C bus speed: 100 kHz. Provide external pull-up resistors to the common
  bus voltage; do not rely on the MCU's weak GPIO pull-ups.
- Slave address: 7-bit `0x30`, matching the serial-to-I2C device with PC4 and
  PC5 both held low.
- Outputs: PA5 is moving, PA6 is stationary, and PA7 is not detected. The
  selected output is driven high and the other two are driven low.
- Programming: Atmel-ICE using UPDI, AVRDUDE programmer `atmelice_updi`,
  target part `t3217`. PA0/UPDI remains reserved.

The master polls the slave status register (`0x00`) every 50 ms. An I2C
timeout or missing slave is treated as not detected and drives PA7 high.
Confirm shared ground, compatible I2C voltage levels, and pull-up placement
on the physical bus.
