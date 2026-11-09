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

Every 5 seconds, the master writes the DFPlayer folder/file convenience
command to the slave, beginning with folder `04`, file `0001`, then advancing
through files `0002` to `0005` and repeating. The write starts at slave
register `0x23` and sends three consecutive values: folder, file, and nonzero
execute. For example, folder 04/file 0001 is sent as:

```text
I2C write to slave 0x30: 23 04 01 01
                         ^^    ^^
                         |     +-- execute register 0x25
                         +-------- register 0x23, then folder 04 and file 01
```

The slave translates this to native DFPlayer command `0x0F` with parameter
`0x0401`. The master does not request a feedback frame; the I2C transaction
only confirms that the command was accepted by the slave's I2C interface.
