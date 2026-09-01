#include "pwm.h"

static uint32_t s_arr; /* current auto-reload value, needed to scale duty_permille */

void pwm_tim3_ch1_init(uint32_t tim_clk_hz, uint32_t pwm_freq_hz)
{
    /* Enable GPIOA and TIM3 clocks */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    /* PA6 = Alternate Function, AF2 = TIM3_CH1 */
    GPIOA->MODER &= ~(0x3UL << (6 * 2));
    GPIOA->MODER |=  (GPIO_MODE_AF << (6 * 2));
    GPIOA->AFR[0] &= ~(0xFUL << (6 * 4));
    GPIOA->AFR[0] |=  (0x2UL << (6 * 4)); /* AF2 = TIM3 */

    /* Prescale down to 1MHz timer clock, then pick ARR for the requested PWM frequency */
    uint32_t timer_tick_hz = 1000000UL;
    TIM3->PSC = (tim_clk_hz / timer_tick_hz) - 1UL;
    s_arr = (timer_tick_hz / pwm_freq_hz) - 1UL;
    TIM3->ARR = s_arr;

    /* Channel 1: PWM mode 1, preload enabled */
    TIM3->CCMR1 &= ~(0x7UL << 4);
    TIM3->CCMR1 |= TIM_CCMR1_OC1M_PWM1 | TIM_CCMR1_OC1PE;
    TIM3->CCER  |= TIM_CCER_CC1E;
    TIM3->CCR1   = 0; /* start at 0% duty */

    TIM3->CR1 |= TIM_CR1_ARPE;
    TIM3->EGR  = 1; /* UG bit: force update to load PSC/ARR immediately */
    TIM3->CR1 |= TIM_CR1_CEN; /* start the counter */
}

void pwm_tim3_ch1_set_duty(uint16_t duty_permille)
{
    if (duty_permille > 1000) duty_permille = 1000;
    /* Scale 0..1000 onto 0..ARR so the duty cycle stays correct regardless of PWM frequency */
    TIM3->CCR1 = (s_arr * duty_permille) / 1000UL;
}
