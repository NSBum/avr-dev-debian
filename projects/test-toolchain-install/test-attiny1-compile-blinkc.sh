#!/usr/bin/env bash

mcus=(attiny1616 attiny3217 attiny412 attiny416 attiny816 attiny3216)
passed=0
failed=0

printf '%-12s %-9s %-24s %-22s\n' 'MCU' 'COMPILE' 'PROGRAM' 'DATA'
printf '%-12s %-9s %-24s %-22s\n' '------------' '---------' '------------------------' '----------------------'

for mcu in "${mcus[@]}"; do
    compile_status='FAIL'
    program='-'
    data='-'

    if avr-gcc \
        -mmcu="$mcu" \
        -DF_CPU=20000000UL \
        -Os \
        -Wall \
        -Wextra \
        -Werror \
        -std=gnu11 \
        -o "/tmp/${mcu}-blink.elf" \
        blink.c >/dev/null 2>&1; then
        size_output=$(avr-size --format=avr --mcu="$mcu" \
            "/tmp/${mcu}-blink.elf" 2>/dev/null) || size_output=''

        if [ -n "$size_output" ]; then
            program=$(printf '%s\n' "$size_output" |
                awk -F: '/^Program:/ { sub(/^[[:space:]]+/, "", $2); print $2 }')
            data=$(printf '%s\n' "$size_output" |
                awk -F: '/^Data:/ { sub(/^[[:space:]]+/, "", $2); print $2 }')
            if [ -n "$program" ] && [ -n "$data" ]; then
                compile_status='PASS'
            fi
        fi
    fi

    if [ "$compile_status" = 'PASS' ]; then
        passed=$((passed + 1))
    else
        failed=$((failed + 1))
    fi

    printf '%-12s %-9s %-24s %-22s\n' \
        "$mcu" "$compile_status" "$program" "$data"
done

printf '\nOverall result: '
if [ "$failed" -eq 0 ]; then
    printf 'PASS (%d/%d MCU targets passed)\n' "$passed" "${#mcus[@]}"
    exit 0
else
    printf 'FAIL (%d/%d MCU targets passed)\n' "$passed" "${#mcus[@]}"
    exit 1
fi
