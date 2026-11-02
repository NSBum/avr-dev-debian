# LD2410C protocol parser

Reusable streaming parser for the LD2410C normal and engineering report
frames. The library does not configure or access a USART; the application
owns serial setup, buffering, and interrupt handling, then passes received
bytes to `tinyavr1_ld2410c_receive_byte()`.

The parser recognizes the LD2410C frame header and tail, validates the basic
report fields, and reports the target state plus moving and stationary
distances in centimeters. The callback executes in the context that calls the
receive function, so it must be short and nonblocking when called from a UART
ISR.

The parser is intended for tinyAVR 1-series applications but contains no
device-specific peripheral code.
