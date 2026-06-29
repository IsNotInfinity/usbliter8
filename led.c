#include <stdio.h>
#include "pico/time.h"
#include "pico/cyw43_arch.h"   // <-- Ahora usamos esto
#include "led.h"

// La Pico 2W tiene el LED conectado al pin 0 del chip Wi-Fi
#define LED_PIN 0   // CYW43_WL_GPIO_LED_PIN también funciona

static struct {
    repeating_timer_t timer;
    bool blink_en;
    bool blink_curr_state;
    repeating_timer_t breathing_timer;
    uint breathing_level;
    bool breathing_falling;
} led_ctx = { 0 };

void led_init(void) {
    // El LED ya se inicializa con cyw43_arch_init() en main.c
    // Solo aseguramos que esté apagado al inicio
    cyw43_arch_gpio_put(LED_PIN, 0);
}

static inline void led_set(bool on) {
    cyw43_arch_gpio_put(LED_PIN, on);
}

// ---- Parpadeo ----
static bool _led_timer_cb(__unused repeating_timer_t *rt) {
    led_set(led_ctx.blink_curr_state);
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
        led_set(false);
        led_ctx.blink_en = false;
    }
}

// ---- Respiración (simulada con parpadeo suave) ----
static bool _led_breathing_timer_cb(__unused repeating_timer_t *rt) {
    // Simulación simple de respiración: encendido/apagado con ciclo variable
    static uint step = 0;
    step++;
    if (step < 10) {
        led_set(step % 2);  // Parpadeo rápido para simular brillo bajo
    } else {
        step = 0;
    }
    return true;
}

void led_set_breathing(bool on) {
    if (on) {
        led_set_blinking(0);
        add_repeating_timer_ms(50, _led_breathing_timer_cb, NULL, &led_ctx.breathing_timer);
    } else {
        cancel_repeating_timer(&led_ctx.breathing_timer);
        led_set(false);
    }
}

// ---- Cambio de estado (según el README) ----
void led_set_state(int state) {
    switch (state) {
        case LED_STATE_BOOTING:   // Parpadeo lento (200ms)
            led_set_blinking(200);
            break;

        case LED_STATE_IDLE:      // Respiración
            led_set_breathing(true);
            break;

        case LED_STATE_RUNNING:   // Parpadeo rápido (100ms)
            led_set_breathing(false);
            led_set_blinking(100);
            break;

        case LED_STATE_SUCCESS:   // Fijo encendido
            led_set_blinking(0);
            led_set(true);
            break;

        case LED_STATE_ERROR:     // Apagado
            led_set_blinking(0);
            led_set(false);
            break;
    }
}