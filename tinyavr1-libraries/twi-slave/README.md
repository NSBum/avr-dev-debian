# tinyAVR 1-series TWI slave

Callback-based TWI0/I²C client for tinyAVR 1-series devices. The library
implements a 7-bit register-pointer protocol:

```text
START, address+W, register pointer,
REPEATED START, address+R, data..., NACK, STOP
```

The first write byte selects the register. Subsequent write bytes invoke the
optional write callback and increment the pointer. Read bytes invoke the read
callback and increment the pointer after each master ACK.

The library uses the device-defined TWI0 default route; verify the exact
device's PORTMUX and pin assignments before use. For ATtiny3216/3217 this is
PB1/SDA and PB0/SCL. External pull-ups and a shared ground are required.

The TWI ISR must remain short because the peripheral may hold SCL low while it
waits for the software response. Call `sei()` in the application after this
library and other interrupt-driven peripherals are initialized.

Compile `tinyavr1_twi_slave.c` into the application and include its directory
when compiling:

```c
#include "tinyavr1_twi_slave.h"

static uint8_t read_register(uint8_t reg)
{
    return application_registers[reg];
}

tinyavr1_twi_slave_init(0x30U, read_register, NULL);
```

The important constraints are:

- Don’t use delays, UART output, or long computations in callbacks.
- Keep callbacks deterministic because they run inside the TWI ISR.
- Avoid non-atomic multi-byte data access; snapshot such values before serving them.
- If maximum speed becomes important, application-specific callbacks could later be replaced with inline register access.