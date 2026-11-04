# tinyAVR 1-series DFPlayer Mini library

This library separates the DFPlayer Mini command/response protocol from the
serial transport. The protocol uses the standard 10-byte frame:

```text
0x7E 0xFF 0x06 command feedback parameter_hi parameter_lo checksum_hi checksum_lo 0xEF
```

Commands use 9600 baud, 8-N-1 through either of these adapters:

- `tinyavr1_dfplayer_transport_soft_uart()` using the reusable software UART;
- `tinyavr1_dfplayer_transport_usart()` using a polling hardware USART.

Example with software serial:

```c
#include "tinyavr1_dfplayer.h"
#include "tinyavr1_dfplayer_soft_uart.h"

static tinyavr1_soft_uart_t serial;
static tinyavr1_dfplayer_t player;

tinyavr1_soft_uart_init(&serial, &PORTA, PIN5_bm, &PORTA, PIN6_bm);
tinyavr1_dfplayer_init(&player,
                       tinyavr1_dfplayer_transport_soft_uart(&serial));
tinyavr1_dfplayer_set_volume(&player, 20U);
    tinyavr1_dfplayer_play_folder_file(&player, 1U, 1U);
```

Example with USART0:

```c
#include "tinyavr1_dfplayer.h"
#include "tinyavr1_dfplayer_usart.h"

tinyavr1_dfplayer_usart_init(&USART0, F_CPU, 9600UL);
tinyavr1_dfplayer_init(&player,
                       tinyavr1_dfplayer_transport_usart(&USART0));
```

The USART adapter is polling and reconfigures the supplied USART, including
its interrupt enable register. The application must configure the appropriate
PORTMUX route and pin directions, and must not use that USART concurrently for
another protocol.

Responses are optional. Set `request_feedback` through
`tinyavr1_dfplayer_send_command()` when a response is wanted, then call
`tinyavr1_dfplayer_read_response()` with a timeout. The `BUSY` pin remains an
independent GPIO input and is not part of the serial protocol.
