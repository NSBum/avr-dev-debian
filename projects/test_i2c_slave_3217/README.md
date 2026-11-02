# Minimal ATtiny3217 I2C slave test

Minimal TWI0 slave diagnostic for isolating the I2C bus and ATtiny3217 TWI
client behavior.

- Address: `0x30`
- Response byte: `0xA5`
- PB1/SDA and PB0/SCL, default TWI0 route
- PA4 high while addressed, low after STOP

Build and flash:

```sh
make
make size
make identify
make flash
make verify
```
