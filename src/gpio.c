#include "gpio.h"

void led_init(void)
{
    /* Enable clock cho GPIOD */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;

    /* Set PD12..PD15 as output (MODER = 01) */
    uint8_t pins[4] = { LED_GREEN_PIN, LED_ORANGE_PIN, LED_RED_PIN, LED_BLUE_PIN };
    for (int i = 0; i < 4; i++) {
        uint8_t pin = pins[i];
        GPIOD->MODER &= ~(0x3UL << (pin * 2));
        GPIOD->MODER |=  (GPIO_MODE_OUTPUT << (pin * 2));
    }
}

void led_on(uint8_t pin)
{
    GPIOD->BSRR = (1UL << pin); /* set bit -> ODR = 1 */
}

void led_off(uint8_t pin)
{
    GPIOD->BSRR = (1UL << (pin + 16)); /* reset bit -> ODR = 0 */
}

void led_toggle(uint8_t pin)
{
    GPIOD->ODR ^= (1UL << pin);
}
