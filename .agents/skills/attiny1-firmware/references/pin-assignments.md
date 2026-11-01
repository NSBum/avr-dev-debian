# tinyAVR 1-series pin assignments

## Scope and reading rules

This reference abstracts the pin assignments in the local Microchip
datasheets. It separates the reset/default position from the alternate
position selected through `PORTMUX`.

The datasheet's superscript footnote `(3)` means **alternate pin position**.
Entries without `(3)` are the **default** positions. This distinction is
important because PDF text extraction can make the superscripts appear to
belong to the wrong signal.

`PxN` means port `x`, pin `N`; it is not a package pin number. Package pin
numbers are listed separately below. A blank package column means that the
port pin is not bonded out on that package.

The following local datasheets were reviewed:

| Datasheet | Devices covered | Multiplexing table |
|---|---|---|
| `docs/datasheets/tinyavr-1/attiny-417-814-816-817.pdf` | ATtiny417, ATtiny814, ATtiny816, ATtiny817 | Table 5-1, p. 18 |
| `docs/datasheets/tinyavr-1/attiny-1614-1616-1617.pdf` | ATtiny1614, ATtiny1616, ATtiny1617 | Table 5-1, p. 18 |
| `docs/datasheets/tinyavr-1/attiny-3216-3216-datasheet.pdf` | ATtiny3216, ATtiny3217 | Table 5-1, p. 16 |
| `docs/datasheets/tinyavr-1/ATtiny212-214-412-414-416-DataSheet-DS40002287A.pdf` | ATtiny212, ATtiny214, ATtiny412, ATtiny414, ATtiny416 | Tables 5-1 and 5-2, pp. 18–19 |

The older `attiny-212-412.pdf` is also present locally, but the newer grouped
DS40002287A datasheet is used for the assignments below.

## PORTMUX controls

| Function | PORTMUX control | `0` | `1` |
|---|---|---|---|
| USART0 | `CTRLB.USART0` | default pins | alternate pins |
| SPI0 | `CTRLB.SPI0` | default pins | alternate pins |
| TWI0 | `CTRLB.TWI0` | default pins | alternate pins |
| TCA0 WO0..WO5 | `CTRLC.TCA00..TCA05` | default pin | alternate pin |
| TCB0 | `CTRLD.TCB0` | default pin | alternate pin |
| TCB1 | `CTRLD.TCB1` | default pin | alternate pin |
| CCL LUT0 output | `CTRLA.LUT0` | default pin | alternate pin |
| CCL LUT1 output | `CTRLA.LUT1` | default pin | alternate pin |

`TCA0 WO3..WO5` alternate selection applies in split mode as described by
the datasheet. `TCA0 WO0..WO2` selection applies in normal mode and to the
corresponding low-byte channels in split mode.

The smaller ATtiny212/214/412/414/416 family has only `TCA00..TCA03` and
only `TCB0`; the eight-pin package has additional bond-out limitations.

## ATtiny212/214/412/414/416

### 14-pin and 20-pin USART0

| Signal | Default | Alternate |
|---|---|---|
| `TxD` | `PB2` | `PA1` |
| `RxD` | `PB3` | `PA2` |
| `XCK` | `PB1` | `PA3` |
| `XDIR` | `PB0` | `PA4` |

### 14-pin and 20-pin SPI0

| Signal | Default | Alternate |
|---|---|---|
| `MOSI` | `PA1` | `PC2` (20-pin) / `PA6` (14-pin) |
| `MISO` | `PA2` | `PC1` (20-pin) / `PA7` (14-pin) |
| `SCK` | `PA3` | `PC0` (20-pin only) |
| `SS` | `PA4` | `PC3` (20-pin only) |

The datasheet explicitly specifies the 14-pin alternate SPI `MOSI` and
`MISO` positions as `PA6` and `PA7`; `PC0..PC3` are not bonded out on the
14-pin package.

### 14-pin and 20-pin TWI0

| Signal | Default | Alternate |
|---|---|---|
| `SDA` | `PB1` | `PA1` |
| `SCL` | `PB0` | `PA2` |

### 14-pin and 20-pin TCA0 waveform outputs

| Signal | Default | Alternate |
|---|---|---|
| `WO0` | `PB0` | `PB3` |
| `WO1` | `PB1` | `PB4` |
| `WO2` | `PB2` | `PB5` |
| `WO3` | `PA3` | `PC3` |
| `WO4` | `PA4` | — |
| `WO5` | `PA5` | — |

The alternate `WO0..WO2` routes require the corresponding `PB3..PB5`
pin, and alternate `WO3` requires `PC3`; therefore those alternate routes
are unavailable on the 14-pin package. `WO4` and `WO5` have no PORTMUX
alternate in this device family.

### 14-pin and 20-pin TCB0, TCD0, and CCL

| Function | Default | Alternate |
|---|---|---|
| TCB0 `WO` | `PA5` | `PC0` (20-pin only) |
| TCD0 `WOA` | `PA4` | — |
| TCD0 `WOB` | `PA5` | — |
| TCD0 `WOC` | — | `PC0` (20-pin only) |
| TCD0 `WOD` | `PC1` | — |
| CCL `LUT0-OUT` | `PA4` | `PB4` |
| CCL `LUT1-OUT` | `PA7` | `PC1` |

The TCD0 `WOC` entry is marked alternate in Table 5-1. The table does not
provide a separate PORTMUX control for TCD0, so preserve the datasheet's
classification and verify the exact TCD configuration before use.

### Eight-pin package

| Peripheral signal | Default | Alternate |
|---|---|---|
| USART0 `TxD` | `PA6` | `PA1` |
| USART0 `RxD` | `PA7` | `PA2` |
| USART0 `XCK` | `PA3` | — |
| USART0 `XDIR` | `PA0` | — |
| SPI0 `MOSI` | `PA1` | `PA6` |
| SPI0 `MISO` | `PA2` | `PA7` |
| SPI0 `SCK` | `PA3` | — |
| SPI0 `SS` | `PA0` | — |
| TWI0 `SDA` | `PA1` | — |
| TWI0 `SCL` | `PA2` | — |
| TCA0 `WO0` | `PA3` | `PA7` |
| TCA0 `WO1` | `PA1` | — |
| TCA0 `WO2` | `PA2` | — |
| TCA0 `WO3` | `PA3` | — |
| TCB0 `WO` | `PA6` | — |
| TCD0 `WOA` | `PA6` | — |
| TCD0 `WOB` | `PA7` | — |
| CCL `LUT0-OUT` | `PA6` | — |
| CCL `LUT1-OUT` | `PA7` | — |

The eight-pin table is Table 5-2. `PA0` remains the `RESET/UPDI` pin even
though it carries default USART0 `XDIR` and SPI0 `SS` functions in that
table.

### Fixed or non-PORTMUX functions

- `PA0`: `RESET/UPDI`; `ADC0 AIN0`.
- `PA2`: `EVOUT0`; `ADC0 AIN2`.
- `PA3`: `EXTCLK`; `ADC0 AIN3`.
- `PB2`: `TOSC2`, `EVOUT1`.
- `PB3`: `TOSC1`.
- `PB5`: `CLKOUT`.
- `PC2`: `EVOUT2`.
- ADC, AC, DAC, and other fixed functions are package/device dependent;
  consult Table 5-1 or Table 5-2 before board layout.

## ATtiny1614/1616/1617 and ATtiny3216/3217

The peripheral pin mapping is the same in these two datasheet families.

### USART0

| Signal | Default | Alternate |
|---|---|---|
| `TxD` | `PB2` | `PA1` |
| `RxD` | `PB3` | `PA2` |
| `XCK` | `PB1` | `PA3` |
| `XDIR` | `PB0` | `PA4` |

### SPI0

| Signal | Default | Alternate |
|---|---|---|
| `MOSI` | `PA1` | `PC2` |
| `MISO` | `PA2` | `PC1` |
| `SCK` | `PA3` | `PC0` |
| `SS` | `PA4` | `PC3` |

### TWI0

| Signal | Default | Alternate |
|---|---|---|
| `SDA` | `PB1` | `PA1` |
| `SCL` | `PB0` | `PA2` |

### TCA0 waveform outputs

| Signal | Default | Alternate |
|---|---|---|
| `WO0` | `PB0` | `PC0` |
| `WO1` | `PB1` | `PC1` |
| `WO2` | `PB2` | `PB5` |
| `WO3` | `PA3` | `PC3` |
| `WO4` | `PA4` | `PC4` |
| `WO5` | `PA5` | `PC5` |

### TCB outputs

| Signal | Default | Alternate |
|---|---|---|
| `TCB0 WO` | `PA5` | `PC0` |
| `TCB1 WO` | `PA3` | `PC4` |

### TCD0 outputs and CCL outputs

| Signal | Default | Alternate |
|---|---|---|
| TCD0 `WOA` | `PA4` | — |
| TCD0 `WOB` | `PA5` | — |
| TCD0 `WOC` | `PC0` | — |
| TCD0 `WOD` | `PC1` | — |
| CCL `LUT0-OUT` | `PA4` | `PB4` |
| CCL `LUT1-OUT` | `PA7` | `PC1` |

TCD0 has no PORTMUX selection in these datasheets. The same physical pin can
appear in more than one peripheral column; select only one active peripheral
function per pin.

### Fixed or non-PORTMUX functions

- `PA0`: `RESET/UPDI`; also `ADC0 AIN0`.
- `PA2`: `EVOUT0`; `ADC0 AIN2`.
- `PA3`: `EXTCLK`; `ADC0 AIN3`.
- `PB2`: `TOSC2`, `EVOUT1`.
- `PB3`: `TOSC1`.
- `PB5`: `CLKOUT`.
- `PC2`: `EVOUT2`.
- ADC, AC, DAC, and PTC functions are assigned to the pins shown in the
  exact-device Table 5-1; PTC is present only where the datasheet specifies.

## ATtiny417/814/816/817

These devices have the same USART0, SPI0, TWI0, and TCA0 routes as above,
but only `TCB0` is present and the CCL set differs.

### USART0, SPI0, and TWI0

The routes are identical to the preceding tables:

| Peripheral | Signal | Default | Alternate |
|---|---|---|---|
| USART0 | `TxD` | `PB2` | `PA1` |
| USART0 | `RxD` | `PB3` | `PA2` |
| USART0 | `XCK` | `PB1` | `PA3` |
| USART0 | `XDIR` | `PB0` | `PA4` |
| SPI0 | `MOSI` | `PA1` | `PC2` |
| SPI0 | `MISO` | `PA2` | `PC1` |
| SPI0 | `SCK` | `PA3` | `PC0` |
| SPI0 | `SS` | `PA4` | `PC3` |
| TWI0 | `SDA` | `PB1` | `PA1` |
| TWI0 | `SCL` | `PB0` | `PA2` |

### TCA0 waveform outputs

| Signal | Default | Alternate |
|---|---|---|
| `WO0` | `PB0` | `PB3` |
| `WO1` | `PB1` | `PB4` |
| `WO2` | `PB2` | `PB5` |
| `WO3` | `PA3` | `PC3` |
| `WO4` | `PA4` | `PC4` |
| `WO5` | `PA5` | `PC5` |

### TCB0, TCD0, and CCL outputs

| Signal | Default | Alternate |
|---|---|---|
| `TCB0 WO` | `PA5` | `PC0` |
| TCD0 `WOA` | `PA4` | — |
| TCD0 `WOB` | `PA5` | — |
| TCD0 `WOC` | `PC0` | — |
| TCD0 `WOD` | `PC1` | — |
| CCL `LUT0-OUT` | `PA4` | `PB4` |
| CCL `LUT1-OUT` | `PA7` | `PC1` |

TCD0 has no PORTMUX selection in this datasheet family. The same physical
pin can appear in more than one peripheral column; select only one active
peripheral function per pin.

### Fixed or non-PORTMUX functions

- `PA0`: `RESET/UPDI`; also `ADC0 AIN0`.
- `PA2`: `EVOUT0`; also `ADC0 AIN2`.
- `PA3`: `EXTCLK`; also `ADC0 AIN3`.
- `PB2`: `TOSC2`, `EVOUT1`.
- `PB3`: `TOSC1`.
- `PB5`: `CLKOUT`.
- `PC2`: `EVOUT2`.
- PTC is available only on the 8 KB-or-more devices in this datasheet family.

## Package pin numbers

Use the port-pin mapping above together with the applicable package column.
These numbers are transcribed from the package columns in Table 5-1.

### 14-pin SOIC

| Port pin | Package pin |
|---|---:|
| `PA0` | 10 |
| `PA1` | 11 |
| `PA2` | 12 |
| `PA3` | 13 |
| `PA4` | 2 |
| `PA5` | 3 |
| `PA6` | 4 |
| `PA7` | 5 |
| `PB0` | 9 |
| `PB1` | 8 |
| `PB2` | 7 |
| `PB3` | 6 |

### 8-pin SOIC

| Port pin | Package pin |
|---|---:|
| `PA0` | 6 |
| `PA1` | 4 |
| `PA2` | 5 |
| `PA3` | 7 |
| `PA6` | 2 |
| `PA7` | 3 |

### 20-pin SOIC / VQFN

| Port pin | 20-pin SOIC | 20-pin VQFN |
|---|---:|---:|
| `PA0` | 16 | 19 |
| `PA1` | 17 | 20 |
| `PA2` | 18 | 1 |
| `PA3` | 19 | 2 |
| `PA4` | 2 | 5 |
| `PA5` | 3 | 6 |
| `PA6` | 4 | 7 |
| `PA7` | 5 | 8 |
| `PB0` | 11 | 16 |
| `PB1` | 10 | 15 |
| `PB2` | 9 | 14 |
| `PB3` | 8 | 13 |
| `PB4` | 7 | 12 |
| `PB5` | 6 | 11 |
| `PC0` | 12 | 17 |
| `PC1` | 13 | 18 |
| `PC2` | 14 | 19 |
| `PC3` | 15 | 20 |

The ATtiny3216/3217 datasheet provides 20-pin SOIC, with the SOIC numbers
shown above. The ATtiny1614/1616/1617 and ATtiny417/814/816/817 datasheets
also provide 20-pin VQFN.

### 24-pin VQFN

| Port pin | Package pin |
|---|---:|
| `PA0` | 23 |
| `PA1` | 24 |
| `PA2` | 1 |
| `PA3` | 2 |
| `PA4` | 5 |
| `PA5` | 6 |
| `PA6` | 7 |
| `PA7` | 8 |
| `PB0` | 16 |
| `PB1` | 15 |
| `PB2` | 14 |
| `PB3` | 13 |
| `PB4` | 12 |
| `PB5` | 11 |
| `PB6` | 10 |
| `PB7` | 9 |
| `PC0` | 17 |
| `PC1` | 18 |
| `PC2` | 19 |
| `PC3` | 20 |
| `PC4` | 21 |
| `PC5` | 22 |

## Review cautions

- `PA0` is the `RESET/UPDI` pin and should be treated as reserved during
  development.
- A route in the multiplexing table does not mean that its port pin is
  bonded out on every package.
- Analog channel names and package pin numbers are device/package specific;
  check the exact Table 5-1 and pinout diagram before board layout.
- The ATtiny212/214/412/414/416 8-pin assignments are package-specific and
  are listed separately above; no 8-pin package exists for the larger
  devices covered by the other grouped datasheets.
