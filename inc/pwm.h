#ifndef __PWM_H
#define __PWM_H

#include "stm32f4xx.h"

/* TIM3 CH1 on PA6 (AF2). Not one of the onboard LED pins, so wire an
   external LED or scope probe to PA6 to see the PWM signal. */
void pwm_tim3_ch1_init(uint32_t tim_clk_hz, uint32_t pwm_freq_hz);

/* duty_permille: 0..1000 (0 = always off, 1000 = always on) */
void pwm_tim3_ch1_set_duty(uint16_t duty_permille);

#endif
