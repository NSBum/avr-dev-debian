# Linux host serial checklist

This reference is for host-side diagnosis. Substitute the stable adapter path for `<port>` in every command.

## Enumerate and identify

```sh
lsusb
find /dev/serial/by-id -maxdepth 1 -type l -print 2>/dev/null
ls -l /dev/ttyUSB* /dev/ttyACM* 2>/dev/null
udevadm info --query=all --name=<port>
```

`/dev/ttyUSB*` is common for USB-UART bridge chips; `/dev/ttyACM*` is common for USB CDC ACM devices. This is only a naming convention. Use USB identity and the by-id symlink to select the actual adapter.

Useful read-only checks:

```sh
id
ls -l <port>
fuser <port> 2>/dev/null
```

If access is denied, report the owning user/group and the current user's groups. A session may need to be restarted after membership changes. Do not use `chmod 666` or permanent root execution as the normal fix.

## Apply a known UART format

For 8 data bits, no parity, one stop bit, no software or hardware flow control:

```sh
stty -F <port> 115200 cs8 -cstopb -parenb -ixon -ixoff -crtscts -echo
stty -F <port> -a
```

Replace `115200` with the firmware's actual baud rate. `stty` cannot correct a wrong MCU clock or baud-rate calculation; it only configures the host side.

## Observe or capture

Start the reader only after configuring the port:

```sh
cat <port>
```

For a bounded diagnostic capture, if the host has `timeout`:

```sh
timeout 10s cat <port> > serial-capture.bin
```

Use a separate decoding step for binary or mixed binary/text protocols. Do not assume every received byte is printable text. A capture file is a user-requested artifact and should be stored in the project’s designated temporary/build area, not committed as source.

## Interpret symptoms

| Symptom | First checks |
|---|---|
| No device node | USB cable, adapter power, `lsusb`, kernel/udev events, driver support |
| Permission denied | Device ownership, `dialout` membership, another process, session refresh |
| Port busy | `fuser`/`lsof`; identify the owner before stopping anything |
| No output | Board power, ground, TX-to-RX, reset timing, correct MCU pin mux, firmware transmit path |
| Repeated `0xff` or noise | Floating input, missing ground, wrong voltage/interface, disconnected TX |
| Consistent gibberish | Baud/clock mismatch, parity/data-bit mismatch, wrong UART instance or electrical standard |
| Missing first lines | Start reader before reset or use a persistent capture |

Do not infer a particular bridge chipset or kernel driver from `/dev/ttyUSB0` alone. Do not transmit test bytes unless the user explicitly requests a bidirectional test and the firmware is known to tolerate them.
