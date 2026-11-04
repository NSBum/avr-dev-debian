# tinyAVR 1-series libraries

Reusable bare-metal C components for Microchip tinyAVR 1-series firmware.

Libraries are independent of the projects under `projects/`; applications
choose their source files and provide device- or board-specific callbacks.

## Libraries

- [`twi-slave/`](twi-slave/): callback-based TWI0/I²C client/slave for the
  default tinyAVR 1-series route.
- [`ld2410c/`](ld2410c/): streaming LD2410C report-frame parser independent of
  the application USART driver.
- [`software-uart/`](software-uart/): pin-configurable cooperative 8-N-1
  software UART for occasional low-speed serial devices.
- [`dfplayer/`](dfplayer/): DFPlayer Mini command/response protocol with
  software-UART and polling-hardware-USART transports.
