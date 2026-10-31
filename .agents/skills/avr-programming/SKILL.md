---
name: avr-programming
description: Program, identify, verify, and diagnose Microchip AVR targets using AVRDUDE and Atmel-ICE over UPDI on Debian Linux. Use for flashing firmware, reading device information, USB permission troubleshooting, and carefully managed fuse operations.
---

# AVR Programming and Verification

## Environment

- Linux: Debian 13
- Programmer: Atmel-ICE
- Interface: UPDI
- Programming tool: AVRDUDE

## Procedure

1. Determine the exact target MCU and AVRDUDE part identifier.
2. Confirm the programmer is visible over USB.
3. Confirm target power and UPDI wiring.
4. Establish communication and verify the device signature.
5. Confirm the firmware was built for the same MCU.
6. Generate an Intel HEX file from the ELF if necessary.
7. Program flash only when explicitly requested.
8. Verify the programmed contents.

## Safety rules

- Never override a mismatched device signature.
- Never modify fuses without explicit approval.
- Never disable or repurpose UPDI without explicit approval.
- Never perform a chip erase unless its consequences are understood and authorized.
- Preserve EEPROM where required.
- Do not assume Atmel-ICE supplies target power.
- Do not use elevated privileges as a permanent workaround for USB permission problems.

## Troubleshooting

Investigate failures in this order:

1. USB enumeration.
2. USB access permissions.
3. Programmer selection.
4. Target supply voltage.
5. Ground and UPDI connections.
6. MCU selection and device signature.
7. Clock and fuse configuration.
8. Programmer firmware and AVRDUDE compatibility.

When AVRDUDE reports an error, consult references/troubleshooting.md before proposing corrective actions. Diagnose failures from USB enumeration through target communication, programming, and application execution. Prefer read-only tests and change one variable at a time.

## Reporting

Distinguish clearly between:

- Programmer detected.
- Target identified.
- Flash written.
- Flash verified.
- Firmware confirmed operational on hardware.

