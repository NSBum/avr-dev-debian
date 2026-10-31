for mcu in attiny1616 attiny3217 attiny412 attiny416 attiny816 attiny3216; do
    if printf '#include <avr/io.h>\nint main(void) { return 0; }\n' |
        avr-gcc -mmcu="$mcu" -x c - -o /tmp/avr-test.elf 2>/dev/null; then
        echo "$mcu: OK"
    else
        echo "$mcu: UNSUPPORTED"
    fi
done
