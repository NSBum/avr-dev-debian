---
name: serial-monitoring
description: Identify and monitor read-only serial output from a USB USART adapter connected to a Linux host. Use for AVR board logs, boot messages, diagnostics, and serial-line troubleshooting; do not use for flashing, fuse changes, or general USB device setup.
---

# Serial Monitoring

Use this skill when an AVR board exposes USART output through a USB-to-serial adapter and the user wants to observe or capture it on Debian Linux.

## Establish the serial contract

Before opening the port, determine or record:

- Adapter device path, preferably a stable `/dev/serial/by-id/...` path rather than a transient `/dev/ttyUSB0` or `/dev/ttyACM0` path.
- UART settings: baud rate, data bits, parity, stop bits, flow control, and expected line endings.
- Electrical interface: TTL/CMOS UART versus RS-232/RS-485, signal voltage, TX/RX crossing, and common ground.
- AVR TX pin, board power state, and whether the adapter is already connected to another application.

If the baud rate or electrical interface is unknown, state the assumption and use read-only observation and incremental diagnosis. A USART-to-USB adapter does not make RS-232 voltage levels safe for a TTL AVR UART, and TX must connect to the adapter's RX.

For AVR firmware timing, consult the project contract and the exact device-firmware documentation when needed. A host capture that shows characters is evidence of a working path, not proof that the MCU USART configuration or application behavior is correct.

## Identify the adapter

Inspect USB enumeration and serial nodes before changing settings:

1. Compare `lsusb` output before and after connecting the adapter when possible.
2. List `/dev/ttyUSB*`, `/dev/ttyACM*`, and `/dev/serial/by-id/*`.
3. Inspect the selected node with `udevadm info --query=all --name=<device>` and `ls -l <device>`.
4. Check ownership and group membership if opening the port fails. On Debian this is commonly the `dialout` group; do not recommend running the monitor as root as a permanent workaround.
5. Confirm that no other process owns the port. Use read-only process inspection such as `fuser <device>` or `lsof <device>` when available.

Prefer the by-id path in commands and project documentation. Never guess a port solely from its number when multiple adapters are present.

## Configure and monitor safely

Configure only the selected serial device and only with the agreed settings. A typical 8-N-1 receive-only setup is:

```sh
stty -F /dev/serial/by-id/<adapter> 115200 cs8 -cstopb -parenb \
    -ixon -ixoff -crtscts -echo
```

Then read from the port with a simple tool available on the host, for example:

```sh
cat /dev/serial/by-id/<adapter>
```

Use a terminal program such as `screen`, `picocom`, `minicom`, or `tio` only if it is installed and its device, baud, and flow-control settings are explicit. Do not send keystrokes, terminal initialization sequences, or control characters unless the user requested bidirectional interaction. When capture is requested, save raw bytes or timestamped text to a user-specified file and preserve the original stream when possible.

Before changing an existing port configuration, understand that `stty` settings are associated with the device and may affect another process. Stop monitoring cleanly and avoid killing unrelated processes.

## Diagnose missing or corrupt output

Use the least invasive checks first:

- No serial node: inspect USB enumeration, kernel/udev events, cable, adapter power, and permissions.
- Port busy: identify the owning process; do not forcibly terminate it without authorization.
- No bytes: verify board power, common ground, TX-to-RX wiring, selected UART pin mux, firmware transmit path, reset timing, and baud agreement.
- Garbled bytes: verify clock assumptions, baud-rate calculation, data format, flow control, voltage level, and adapter driver; distinguish an incorrect line rate from electrical noise.
- Output starts only after reset: open the monitor before resetting the board, or use a capture method that does not miss the boot banner.

Do not change AVR fuses, clock settings, pin assignments, or firmware merely to make monitoring work without explicit authorization. Do not claim hardware execution from a successful `cat`, terminal connection, or USB enumeration alone.

## Reporting

Report:

- Adapter identity and stable device path.
- UART settings and electrical/wiring assumptions.
- Host checks performed and any permission or port-conflict findings.
- Whether live bytes were observed, captured, and decoded as expected.
- Tests not run and hardware facts that remain unverified.

For firmware tasks, combine this report with the AVR build/programming report. Keep monitoring separate from flashing and never flash automatically as part of this skill.

## Supporting reference

Read [references/host-serial.md](references/host-serial.md) for the Linux command checklist and common failure interpretations when the task requires detailed host-side diagnosis.
