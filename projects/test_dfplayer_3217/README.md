# DFPlayer Mini standalone test

Minimal ATtiny3217 project that only tests communication with a DFPlayer Mini.
It does not configure the radar USART, I²C, or radar parser.

At 20 MHz, the firmware uses the reusable software UART at 9600 baud and
attempts to play:

```text
01/001.mp3
```

every five seconds. It requests and validates a DFPlayer response. A valid
response is treated as success; a timeout, serial framing error, malformed
response, checksum error, or DFPlayer error response causes PA4 to flash three
times quickly.

## Pins

- PA4: error indicator, active high
- PA5: software UART TX to DFPlayer RX
- PA6: software UART RX from DFPlayer TX
- PA7: DFPlayer BUSY input, active low while playing
- PA0: UPDI

Use the required 3.3 V/5 V level translation and connect grounds together.
Connect the speaker between DFPlayer `SPK1` and `SPK2`.

Build and flash with:

```sh
make
make size
make flash
make verify
```
