# Hardware contract

- Target: Microchip ATtiny3217 in the 24-pin VQFN package. PB5 and PB6 are
  only both available on this package; they are physical pins 11 and 10.
- Outputs: LEDs (with suitable current-limiting circuitry) connected to PB5
  and PB6. The firmware assumes active-high LEDs.
- Clock: internal OSC20M, selected as the main clock, with no prescaler.
  The existing `FUSE.OSCCFG` setting must select 20 MHz to match
  `F_CPU=20000000UL`. Firmware does not alter fuses.
- Programming: Atmel-ICE connected over UPDI. Connect target power and ground
  correctly; Atmel-ICE target power is not assumed. PA0 remains reserved for
  UPDI and is not configured by the application.
