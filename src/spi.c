#include "spi.h"

void spi1_init(void)
{
    /* Enable GPIOA (for CS), GPIOB (for SCK/MISO/MOSI) and SPI1 clocks */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    /* PB3=SCK, PB4=MISO, PB5=MOSI, Alternate Function, AF5 = SPI1 */
    uint8_t pins[3] = { 3, 4, 5 };
    for (int i = 0; i < 3; i++) {
        uint8_t pin = pins[i];
        GPIOB->MODER &= ~(0x3UL << (pin * 2));
        GPIOB->MODER |=  (GPIO_MODE_AF << (pin * 2));
        GPIOB->OSPEEDR |= (0x3UL << (pin * 2)); /* high speed */

        uint8_t af_reg = pin / 8;
        uint8_t af_shift = (pin % 8) * 4;
        GPIOB->AFR[af_reg] &= ~(0xFUL << af_shift);
        GPIOB->AFR[af_reg] |=  (0x5UL << af_shift); /* AF5 = SPI1 */
    }

    /* PA4 = CS, plain push-pull output, idle high (card not selected) */
    GPIOA->MODER &= ~(0x3UL << (4 * 2));
    GPIOA->MODER |=  (GPIO_MODE_OUTPUT << (4 * 2));
    spi1_cs_high();

    /* Master mode, CPOL=0/CPHA=0 (SPI mode 0, required by SD cards),
       software slave management (SSM=1, SSI=1) since CS is handled manually. */
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_BR_DIV256;
    SPI1->CR1 |= SPI_CR1_SPE;
}

void spi1_set_baudrate_slow(void)
{
    SPI1->CR1 &= ~SPI_CR1_SPE;
    SPI1->CR1 = (SPI1->CR1 & ~(0x7UL << 3)) | SPI_CR1_BR_DIV256;
    SPI1->CR1 |= SPI_CR1_SPE;
}

void spi1_set_baudrate_fast(void)
{
    SPI1->CR1 &= ~SPI_CR1_SPE;
    SPI1->CR1 = (SPI1->CR1 & ~(0x7UL << 3)) | SPI_CR1_BR_DIV4;
    SPI1->CR1 |= SPI_CR1_SPE;
}

uint8_t spi1_transfer_byte(uint8_t out)
{
    while ((SPI1->SR & SPI_SR_TXE) == 0) { /* wait until the TX buffer is empty */
    }
    SPI1->DR = out;
    while ((SPI1->SR & SPI_SR_RXNE) == 0) { /* wait until a byte has been shifted in */
    }
    return (uint8_t)SPI1->DR;
}

void spi1_cs_low(void)
{
    GPIOA->BSRR = (1UL << (4 + 16)); /* reset PA4 */
}

void spi1_cs_high(void)
{
    GPIOA->BSRR = (1UL << 4); /* set PA4 */
}
