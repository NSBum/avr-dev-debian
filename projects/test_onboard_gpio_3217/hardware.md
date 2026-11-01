# Hardware contract

- Target: Microchip ATtiny3217, 24-pin VQFN package assumed.
- Board: board name/revision not supplied; PA4 is assumed to be connected to
  the onboard LED or other suitable indicator.
- Supply: must support the device's 20 MHz operating limit. The datasheet
  specifies 20 MHz operation at 4.5 V--5.5 V; confirm the board voltage before
  use.
- Clock: internal OSC20M, nominal 20 MHz. `FUSE.OSCCFG` must select 20 MHz;
  firmware disables the main clock prescaler. No clock fuse is changed.
- GPIO: PA4 is initialized low before being configured as an output, then
  toggled every 500 ms. The application assumes an active-high indicator;
  an active-low LED will appear inverted but will have the same period.
- Programming: Atmel-ICE using UPDI, AVRDUDE programmer `atmelice_updi`,
  target part `t3217`. PA0/UPDI remains reserved.

The exact board LED wiring, package population, supply voltage, and oscillator
fuse state remain to be checked on the physical board.
