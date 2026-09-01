#ifndef __GPIO_H
#define __GPIO_H

#include "stm32f4xx.h"

/* Discovery board: LEDs on PD12 (green), PD13 (orange), PD14 (red), PD15 (blue) */
#define LED_GREEN_PIN   12
#define LED_ORANGE_PIN  13
#define LED_RED_PIN     14
#define LED_BLUE_PIN    15

void led_init(void);
void led_on(uint8_t pin);
void led_off(uint8_t pin);
void led_toggle(uint8_t pin);

#endif
