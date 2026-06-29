#include <stdio.h>
#include "pico/time.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "led.h"

#ifndef LED_PIN
#error "LED_PIN must be defined"
#endif

#define PWM_WRAP 768

static struct {
    uint slice;
    uint chan;
    repeating_timer_t timer;
    bool blink_en;
    bool blink_curr_state;
    repeating_timer_t breathing_timer;
    uint breathing_level;
    bool breathing_falling;
} led_ctx = { 0 };

void led_init(void) {
    gpio_set_function(LED_PIN, GPIO_FUNC_PWM);
    led_ctx.slice = pwm_gpio_to_slice_num(LED_PIN);
    led_ctx.chan = pwm_gpio_to_channel(LED_PIN);
    pwm_set_wrap(led_ctx.slice, PWM_WRAP);
    pwm_set_chan_level(led_ctx.slice, led_ctx.chan, PWM_WRAP);
    pwm_set_enabled(led_ctx.slice, true);
}

#define ENABLED  (PWM_WRAP - 1)
#define DISABLED (0)

static inline void led_toggle(bool on) {
    pwm_set_chan_level(led_ctx.slice, led_ctx.chan, on ? ENABLED : DISABLED);
}

static bool _led_timer_cb(__unused repeating_timer_t *rt) {
    led_toggle(led_ctx.blink_curr_state);
    led_ctx.blink_curr_state = !led_ctx.blink_curr_state;
    return true;
}

void led_set_blinking(uint64_t period_ms) {
    if (led_ctx.blink_en) {
        cancel_repeating_timer(&led_ctx.timer);
    }
    if (period_ms) {
        add_repeating_timer_ms(period_ms, _led_timer_cb, NULL, &led_ctx.timer);
        led_ctx.blink_en = true;
    } else {
        led_toggle(false);
        led_ctx.blink_en = false;
    }
}

static bool _led_breathing_timer_cb(__unused repeating_timer_t *rt) {
    if (!led_ctx.breathing_falling) {
        led_ctx.breathing_level++;
        if (led_ctx.breathing_level == ENABLED)
            led_ctx.breathing_falling = true;
    } else {
        led_ctx.breathing_level--;
        if (led_ctx.breathing_level == 0)
            led_ctx.breathing_falling = false;
    }
    pwm_set_chan_level(led_ctx.slice, led_ctx.chan, led_ctx.breathing_level);
    return true;
}

void led_set_breathing(bool on) {
    if (on) {
        led_set_blinking(0);
        add_repeating_timer_ms(2, _led_breathing_timer_cb, NULL, &led_ctx.breathing_timer);
    } else {
        cancel_repeating_timer(&led_ctx.breathing_timer);
        led_toggle(DISABLED);
    }
}

void led_set_state(int state) {
    switch (state) {
        case LED_STATE_BOOTING:
            led_set_blinking(200);
            break;
        case LED_STATE_IDLE:
            led_set_breathing(true);
            break;
        case LED_STATE_RUNNING:
            led_set_breathing(false);
            led_set_blinking(100);
            break;
        case LED_STATE_SUCCESS:
            led_set_blinking(0);
            led_toggle(ENABLED);
            break;
        case LED_STATE_ERROR:
            led_set_blinking(0);
            led_toggle(DISABLED);
            break;
    }
}