# Hardware contract

- Target: Microchip ATtiny3217, 24-pin package.
- Clock: internal OSC20M, nominal 20 MHz. `FUSE.OSCCFG` must select 20 MHz;
  firmware disables the main clock prescaler. No clock fuse is changed.
- Indicator: PA4, active-high LED or other suitable load.
- USART0 default route: PB2/TXD and PB3/RXD. The firmware uses PB3/RXD;
  PB2/TXD is left unused.
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
