# tinyAVR 1-series libraries

Reusable bare-metal C components for Microchip tinyAVR 1-series firmware.

Libraries are independent of the projects under `projects/`; applications
choose their source files and provide device- or board-specific callbacks.

## Libraries

- [`twi-slave/`](twi-slave/): callback-based TWI0/I²C client/slave for the
  default tinyAVR 1-series route.
