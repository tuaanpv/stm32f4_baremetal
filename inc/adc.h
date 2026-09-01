#ifndef __ADC_H
#define __ADC_H

#include "stm32f4xx.h"

/* Read ADC1 channel 0 = PA0 on the Discovery board */
void adc1_init(void);
uint16_t adc1_read(void);

#endif
