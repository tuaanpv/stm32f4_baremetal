#include "adc.h"

void adc1_init(void)
{
    /* Enable GPIOA and ADC1 clocks */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

    /* PA0 = analog mode (MODER = 11) */
    GPIOA->MODER |= (GPIO_MODE_ANALOG << (0 * 2));

    /* Long sample time for channel 0 (SMPR2 bits 2:0), safe for high-impedance inputs */
    ADC1->SMPR2 |= (0x7UL << 0);

    /* 1 conversion in the sequence, channel 0 */
    ADC1->SQR3 = 0; /* channel 0 */
    ADC1->SQR1 &= ~(0xFUL << 20); /* L[3:0] = 0 -> 1 conversion */

    /* Enable ADC */
    ADC1->CR2 |= ADC_CR2_ADON;
}

uint16_t adc1_read(void)
{
    ADC1->CR2 |= ADC_CR2_SWSTART;          /* start conversion */
    while ((ADC1->SR & ADC_SR_EOC) == 0) { /* wait for conversion to finish */
    }
    return (uint16_t)(ADC1->DR & 0xFFFF);  /* reading DR also clears EOC */
}
