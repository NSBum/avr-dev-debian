# AVRDUDE and Atmel-ICE Troubleshooting

## Scope

This reference covers programming **all 14 Microchip tinyAVR 1-series devices** from Debian Linux using AVRDUDE, an Atmel-ICE programmer, and the UPDI interface.

The complete supported device family is:

| Device | GCC `-mmcu` | AVRDUDE `-p` |
|---|---|---|
| ATtiny212 | `attiny212` | `t212` |
| ATtiny214 | `attiny214` | `t214` |
| ATtiny412 | `attiny412` | `t412` |
| ATtiny414 | `attiny414` | `t414` |
| ATtiny416 | `attiny416` | `t416` |
| ATtiny417 | `attiny417` | `t417` |
| ATtiny814 | `attiny814` | `t814` |
| ATtiny816 | `attiny816` | `t816` |
| ATtiny817 | `attiny817` | `t817` |
| ATtiny1614 | `attiny1614` | `t1614` |
| ATtiny1616 | `attiny1616` | `t1616` |
| ATtiny1617 | `attiny1617` | `t1617` |
| ATtiny3216 | `attiny3216` | `t3216` |
| ATtiny3217 | `attiny3217` | `t3217` |

### Target selection rules

1. All 14 devices listed above are within the scope of this reference. None should be treated as unsupported merely because it is not used in an example.
2. Always determine the exact target MCU before compiling, programming, or troubleshooting.
3. Use the corresponding GCC `-mmcu` identifier when building firmware.
4. Use the corresponding AVRDUDE `-p` identifier when programming, after confirming that it is recognized by the installed AVRDUDE version.
5. All devices use UPDI for programming, but pinouts, memory capacities, and peripheral capabilities vary.
6. Never assume that firmware compiled for one family member is suitable for another.
7. Commands elsewhere in this reference using ATtiny1616 are illustrative only. Substitute the actual target device identifier.
8. Do not confuse tinyAVR 1-series devices with the separate tinyAVR 0-series or 2-series families.
9. If an expected device identifier is not recognized, investigate the installed toolchain or AVRDUDE configuration rather than assuming the MCU is unsupported.

The exact device datasheet and installed toolchain definitions take precedence over generic family-level assumptions.

## 1. Establish the environment

Collect:

```bash
cat /etc/os-release
avr-gcc --version
avrdude -v
lsusb
```

Confirm the installed AVRDUDE recognizes the programmer and target identifiers:

```bash
avrdude -c '?' 2>&1
avrdude -p '?' 2>&1
```

The expected programmer for Atmel-ICE UPDI programming is `atmelice_updi`.

For the ATtiny1616, the expected part identifier is `t1616`.

Use the locally installed AVRDUDE configuration as the authority. Do not assume identifiers from another version.

## 2. Programmer not detected over USB

### Symptoms

- Atmel-ICE is absent from `lsusb`.
- AVRDUDE reports that it cannot find or open the programmer.
- USB enumeration errors appear in kernel logs.

### Diagnostics

```bash
lsusb
sudo dmesg -T | tail -80
```

Alternatively:

```bash
sudo journalctl -k -n 80 --no-pager
```

### Actions

1. Check that the USB cable supports data transfer.
2. Connect directly to the host rather than through a USB hub.
3. Try another USB port or cable.
4. Inspect kernel messages while reconnecting the programmer.
5. Confirm the programmer is receiving USB power.

Do not troubleshoot UPDI wiring until the host recognizes the programmer.

## 3. USB permission denied

### Symptoms

- The programmer appears in `lsusb`.
- AVRDUDE reports permission denied or cannot open the USB device.
- Programming succeeds with elevated privileges but fails as the normal user.

### Diagnostics

Identify the programmer's USB vendor and product IDs using:

```bash
lsusb
```

Inspect the corresponding USB device node:

```bash
ls -l /dev/bus/usb/BBB/DDD
```

Replace BBB and DDD with the actual bus and device numbers.

### Actions

Configure a persistent udev rule using the confirmed vendor and product IDs.

Do not invent USB identifiers.

A typical rule structure is:

```udev
SUBSYSTEM=="usb", ATTR{idVendor}=="VVVV", ATTR{idProduct}=="PPPP", MODE="0660", GROUP="plugdev", TAG+="uaccess"
```

Adapt the rule to the actual device and local permission model.

Reload rules:

```bash
sudo udevadm control --reload-rules
```

Reconnect the programmer and test again.

Do not use world-writable USB permissions as a default solution.

Do not recommend running AVRDUDE permanently as root.

## 4. Programmer detected but target communication fails

### Symptoms

- AVRDUDE opens the Atmel-ICE.
- The target cannot be identified.
- UPDI initialization or communication fails.
- AVRDUDE reports target voltage problems.

### Diagnostics

Check the physical connections:

| Signal | Requirement |
|---|---|
| VTG | Connected to target VCC |
| GND | Common ground |
| UPDI | Connected to the correct target UPDI pin |

Confirm the exact Atmel-ICE connector pinout from official documentation.

Measure the target supply voltage with a multimeter.

Verify that the supply is within the operating limits of both the MCU and programmer.

### Actions

1. Confirm the target is powered.
2. Confirm common ground.
3. Verify UPDI wiring and connector orientation.
4. Check for shorts or incorrect pin assignments.
5. Check for external circuitry loading the UPDI signal.
6. Reduce UPDI communication speed if supported and justified.
7. Retry device identification.

Do not assume Atmel-ICE supplies target power.

Do not modify fuses to solve an unexplained wiring problem.

## 5. Incorrect device signature

### Symptoms

- AVRDUDE reads a device signature.
- The signature differs from the expected target.
- AVRDUDE refuses to continue.

### Diagnostics

Verify:

1. The exact part number printed on the MCU.
2. The AVRDUDE `-p` identifier.
3. The device signature expected by the installed AVRDUDE configuration.
4. The actual reported signature.

### Actions

Correct the MCU selection if necessary.

Investigate electrical or communication problems if the signature is inconsistent or unstable.

Never use `-F` to override a mismatched signature.

## 6. UPDI pin unavailable or reconfigured

### Symptoms

- The target previously programmed successfully.
- UPDI communication stopped after a configuration change.
- The UPDI pin may have been reassigned using fuse settings.

### Diagnostics

Review recent changes to:

- SYSCFG0 fuse configuration.
- UPDI/RESET/GPIO pin selection.
- External circuitry attached to the UPDI pin.

### Actions

Determine whether recovery requires a supported high-voltage UPDI activation procedure.

Do not assume an ordinary UPDI programming attempt can recover a disabled UPDI interface.

Do not apply high voltage without confirming the target device, programming hardware, electrical connections, and recovery procedure.

Do not assume Atmel-ICE supports every high-voltage recovery operation.

## 7. Flash programming fails

### Symptoms

- Device identification succeeds.
- Flash writing fails or terminates unexpectedly.
- Verification reports differences.

### Diagnostics

Confirm:

1. The firmware was compiled for the correct MCU.
2. The HEX file exists and is valid.
3. The firmware fits in target flash.
4. The target voltage remains stable.
5. UPDI communication is reliable.
6. The device is not protected against the requested operation.

Inspect firmware size:

```bash
avr-size -G --mcu=attiny1616 firmware.elf
```

### Actions

- Rebuild the firmware.
- Confirm the correct HEX file is being programmed.
- Investigate target supply instability.
- Check UPDI wiring and communication reliability.
- Inspect protection or lock settings using supported read operations.

Do not disable verification using `-V`.

Do not automatically erase the device as a troubleshooting shortcut.

## 8. Programming succeeds but firmware does not run

A successful flash verification does not prove application correctness.

### Clock configuration

Check:

- Actual oscillator selection.
- OSCCFG fuse configuration.
- Main clock prescaler.
- `F_CPU` build definition.
- Timing-dependent peripheral initialization.

For a nominal 20 MHz internal oscillator, verify that the oscillator fuse selects 20 MHz and that the prescaler is disabled.

Use CCP-protected writes where required.

### GPIO

Check:

- Physical pin mapping.
- Port direction configuration.
- Output polarity.
- External pull-up or pull-down resistors.
- LED polarity and current-limiting resistor.
- Peripheral ownership or PORTMUX configuration.

### Interrupts

Check:

- Correct interrupt vector.
- Peripheral interrupt enable.
- Global interrupt enable.
- Interrupt flag handling.
- Shared-variable concurrency.
- ISR execution time.

### Watchdog and reset

Investigate:

- Watchdog configuration.
- Brown-out detection.
- Supply voltage instability.
- Reset cause registers.

Capture reset-cause information early during startup when relevant.

## 9. Timing is incorrect

### Symptoms

- LED blink frequency is incorrect.
- USART baud rate is incorrect.
- Timer periods differ from calculations.
- Delay functions produce unexpected results.

### Diagnostics

1. Verify actual oscillator frequency selection.
2. Verify clock prescaler settings.
3. Verify `F_CPU`.
4. Check peripheral-specific prescalers.
5. Check timer counter modes and period registers.
6. Check integer arithmetic and overflow.
7. Check compiler optimization and generated instructions.

Remember that the internal oscillator has frequency tolerance and drift.

Do not infer actual CPU frequency solely from the value of `F_CPU`.

## 10. EEPROM unexpectedly changes

Investigate:

- Whether a chip erase occurred.
- Whether EEPROM preservation is configured.
- Whether firmware writes EEPROM during startup.
- Whether power loss interrupted an EEPROM operation.

Read and preserve EEPROM before potentially destructive operations when its contents matter.

Do not assume EEPROM survives every programming or erase procedure.

## 11. Fuse-related problems

Before proposing a fuse change:

1. Identify the exact MCU.
2. Read and record existing fuse values.
3. Consult the exact device datasheet.
4. Determine the desired value and affected bits.
5. Explain potential loss of programming access.
6. Obtain explicit authorization.
7. Write only the intended fuse or configuration byte.
8. Read back and verify the result.

Never infer fuse values from a different AVR family.

Never modify unrelated fuse bits.

## 12. Escalation procedure

If the failure remains unresolved, prepare a diagnostic report containing:

- Host OS and kernel version.
- AVRDUDE version.
- Programmer model.
- USB vendor and product IDs.
- Exact MCU part number.
- Target supply voltage.
- UPDI wiring description.
- Complete AVRDUDE command.
- Full command output.
- Whether device identification succeeds.
- Whether flash write succeeds.
- Whether verification succeeds.
- Recent hardware or fuse changes.

State the most likely causes, the evidence supporting each, and the next safest diagnostic action.

Do not claim a cause is established when it remains a hypothesis.

