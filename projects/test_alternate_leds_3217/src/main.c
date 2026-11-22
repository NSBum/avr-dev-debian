#include <avr/cpufunc.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdint.h>

#include "board.h"
#include "config.h"

static volatile uint16_t switch_ticks;

static void clock_init(void)
{
    /* FUSE.OSCCFG must already select the 20 MHz OSC20M configuration. */
    _PROTECTED_WRITE(CLKCTRL.MCLKCTRLA, CLKCTRL_CLKSEL_OSC20M_gc);
    _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, 0);
}

static void leds_init(void)
{
    /* Set PB5 on and PB6 off before enabling their output drivers. */
    LED_PORT.OUTSET = LED_PB5_bm;
    LED_PORT.OUTCLR = LED_PB6_bm;
    LED_PORT.DIRSET = LED_PINS_bm;
}

static void timer_init(void)
{
    TCB0.CTRLA = 0;
    TCB0.CTRLB = TCB_CNTMODE_INT_gc;
    TCB0.CCMP = TCB0_PERIOD;
    TCB0.INTFLAGS = TCB_CAPT_bm;
    TCB0.INTCTRL = TCB_CAPT_bm;
    TCB0.CTRLA = TCB_CLKSEL_CLKDIV2_gc | TCB_ENABLE_bm;
}

ISR(TCB0_INT_vect)
{
    TCB0.INTFLAGS = TCB_CAPT_bm;

    ++switch_ticks;
    if (switch_ticks >= LED_SWITCH_TICKS) {
        switch_ticks = 0;
        LED_PORT.OUTTGL = LED_PINS_bm;
    }
}

int main(void)
{
    clock_init();
    leds_init();
    timer_init();
    sei();

    for (;;) {
    }
}
