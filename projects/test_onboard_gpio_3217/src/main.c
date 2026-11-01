#include <avr/cpufunc.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdint.h>

#include "board.h"
#include "config.h"

static volatile uint16_t toggle_ticks;

static void clock_init(void)
{
    /* Select OSC20M; FUSE.OSCCFG must select its 20 MHz setting. */
    _PROTECTED_WRITE(CLKCTRL.MCLKCTRLA, CLKCTRL_CLKSEL_OSC20M_gc);

    /* Run CPU and synchronous peripherals without the reset prescaler. */
    _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, 0);
}

static void gpio_init(void)
{
    /* Set the latch first so enabling output cannot create a glitch. */
    ONBOARD_GPIO_PORT.OUTCLR = ONBOARD_GPIO_PIN;
    ONBOARD_GPIO_PORT.DIRSET = ONBOARD_GPIO_PIN;
}

static void timer_init(void)
{
    TCB0.CTRLA = 0;
    TCB0.CTRLB = TCB_CNTMODE_INT_gc;
    TCB0.CCMP = TCB0_PERIOD;

    /* Clear a stale flag before enabling the interrupt and timer. */
    TCB0.INTFLAGS = TCB_CAPT_bm;
    TCB0.INTCTRL = TCB_CAPT_bm;
    TCB0.CTRLA = TCB_CLKSEL_CLKDIV2_gc | TCB_ENABLE_bm;
}

ISR(TCB0_INT_vect)
{
    TCB0.INTFLAGS = TCB_CAPT_bm;

    ++toggle_ticks;
    if (toggle_ticks >= GPIO_TOGGLE_TICKS) {
        toggle_ticks = 0;
        ONBOARD_GPIO_PORT.OUTTGL = ONBOARD_GPIO_PIN;
    }
}

int main(void)
{
    clock_init();
    gpio_init();
    timer_init();
    sei();

    for (;;) {
        /* All work is timer-driven. */
    }
}
