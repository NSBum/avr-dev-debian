
#define F_CPU 20000000UL

#include <avr/io.h>
#include <avr/cpufunc.h>
#include <util/delay.h>
#include <stdint.h>

#define LED_PIN PIN3_bm

static void clock_to_20mhz_no_prescale(void)
{
    // Select the internal 16/20 MHz oscillator.
    _PROTECTED_WRITE(CLKCTRL.MCLKCTRLA,
                     CLKCTRL_CLKSEL_OSC20M_gc);

    // Disable the main clock prescaler.
    _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, 0);
}

static void gpio_init(void)
{
    // Configure PA3 as output.
    PORTA.DIRSET = LED_PIN;

    // Initially drive PA3 low.
    PORTA.OUTCLR = LED_PIN;
}

int main(void)
{
    clock_to_20mhz_no_prescale();
    gpio_init();

    while (1) {
        // Toggle PA3.
        PORTA.OUTTGL = LED_PIN;

        _delay_ms(500);
    }
}

