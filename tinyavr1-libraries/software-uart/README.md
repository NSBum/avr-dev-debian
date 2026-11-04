# tinyAVR 1-series software UART

This library provides a pin-configurable, timer-free 8-N-1 software UART for
tinyAVR 1-series devices. It supports:

- transmit one byte or a buffer;
- blocking receive;
- polling receive with `NO_DATA`; and
- receive with a millisecond timeout.

The pins are supplied to `tinyavr1_soft_uart_init()`, so any GPIO pins that
are physically available may be selected:

```c
static tinyavr1_soft_uart_t dfplayer_uart;

tinyavr1_soft_uart_init(&dfplayer_uart,
                        &PORTA, PIN5_bm, /* TX */
                        &PORTA, PIN6_bm); /* RX */

tinyavr1_soft_uart_putc(&dfplayer_uart, 0x7E);
```

The default baud rate is 9600. Override `TINYAVR1_SOFT_UART_BAUD` at compile
time before including the header when another rate is required. The protocol
is fixed at 8 data bits, no parity, and one stop bit.

This is a cooperative, blocking implementation. It does not allocate a timer,
interrupt vector, receive ring buffer, or callback. That makes it easy to use
for occasional devices such as a DFPlayer Mini, but it is not a drop-in
replacement for an interrupt-driven hardware USART:

- `putc()` occupies the caller for approximately 10 bit times;
- receive functions occupy the caller for one complete frame;
- the caller must service reception often enough to see the start bit; and
- interrupt latency and other long-running ISRs add sampling jitter.

At 20 MHz and 9600 baud, this is normally suitable for short, occasional
messages. Keep the software UART pins at idle-high when disconnected and use
appropriate level translation for the attached device.
