#include "i2c.h"

#define I2C_TIMEOUT 100000UL

static int wait_flag(volatile uint32_t *reg, uint32_t mask)
{
    uint32_t t = I2C_TIMEOUT;
    while (((*reg) & mask) == 0) {
        if (--t == 0) return -1; /* timeout -> avoid hanging the program if the bus fails */
    }
    return 0;
}

void i2c1_init(void)
{
    /* Enable GPIOB and I2C1 clocks */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

    /* PB6 = SCL, PB9 = SDA -> Alternate Function, Open-drain, Pull-up, AF4 (I2C1) */
    for (int i = 0; i < 2; i++) {
        uint8_t pin = (i == 0) ? 6 : 9;
        GPIOB->MODER   &= ~(0x3UL << (pin * 2));
        GPIOB->MODER   |=  (GPIO_MODE_AF << (pin * 2));
        GPIOB->OTYPER  |=  (1UL << pin);         /* open-drain */
        GPIOB->PUPDR   &= ~(0x3UL << (pin * 2));
        GPIOB->PUPDR   |=  (0x1UL << (pin * 2)); /* pull-up */
        GPIOB->OSPEEDR |=  (0x3UL << (pin * 2)); /* high speed */

        uint8_t af_reg  = pin / 8;
        uint8_t af_shift = (pin % 8) * 4;
        GPIOB->AFR[af_reg] &= ~(0xFUL << af_shift);
        GPIOB->AFR[af_reg] |=  (0x4UL << af_shift); /* AF4 = I2C1 */
    }

    /* Reset I2C1 before configuring it */
    I2C1->CR1 = (1UL << 15); /* SWRST */
    I2C1->CR1 = 0;

    /* APB1 clock = 16MHz (default HSI, PLL not configured in this example) */
    I2C1->CR2 = 16; /* FREQ[5:0] = 16 MHz */

    /* Standard mode 100kHz: CCR = Fpclk1 / (2 * Fi2c) */
    I2C1->CCR = 80;      /* 16MHz / (2*100kHz) = 80 */
    I2C1->TRISE = 17;    /* (1000ns / (1/16MHz)) + 1 */

    I2C1->CR1 |= I2C_CR1_PE; /* enable peripheral */
}

static int i2c1_start(uint8_t dev_addr, uint8_t read)
{
    I2C1->CR1 |= I2C_CR1_ACK;
    I2C1->CR1 |= I2C_CR1_START;
    if (wait_flag(&I2C1->SR1, I2C_SR1_SB) < 0) return -1;

    I2C1->DR = (uint8_t)((dev_addr << 1) | (read ? 1 : 0));
    if (wait_flag(&I2C1->SR1, I2C_SR1_ADDR) < 0) return -1;
    (void)I2C1->SR1;
    (void)I2C1->SR2; /* clear ADDR by reading SR1 then SR2 */
    return 0;
}

static void i2c1_stop(void)
{
    I2C1->CR1 |= I2C_CR1_STOP;
}

int i2c1_write(uint8_t dev_addr, uint8_t reg, const uint8_t *data, uint16_t len)
{
    if (i2c1_start(dev_addr, 0) < 0) return -1;

    if (wait_flag(&I2C1->SR1, I2C_SR1_TXE) < 0) { i2c1_stop(); return -1; }
    I2C1->DR = reg;

    for (uint16_t i = 0; i < len; i++) {
        if (wait_flag(&I2C1->SR1, I2C_SR1_TXE) < 0) { i2c1_stop(); return -1; }
        I2C1->DR = data[i];
    }

    if (wait_flag(&I2C1->SR1, I2C_SR1_BTF) < 0) { i2c1_stop(); return -1; }
    i2c1_stop();
    return 0;
}

int i2c1_read(uint8_t dev_addr, uint8_t reg, uint8_t *data, uint16_t len)
{
    /* Send the register address first (short write with no data) */
    if (i2c1_start(dev_addr, 0) < 0) return -1;
    if (wait_flag(&I2C1->SR1, I2C_SR1_TXE) < 0) { i2c1_stop(); return -1; }
    I2C1->DR = reg;
    if (wait_flag(&I2C1->SR1, I2C_SR1_TXE) < 0) { i2c1_stop(); return -1; }

    /* Repeated START, then read */
    if (i2c1_start(dev_addr, 1) < 0) return -1;

    for (uint16_t i = 0; i < len; i++) {
        if (i == len - 1) {
            I2C1->CR1 &= ~I2C_CR1_ACK; /* NACK the last byte */
        }
        if (wait_flag(&I2C1->SR1, I2C_SR1_RXNE) < 0) { i2c1_stop(); return -1; }
        data[i] = (uint8_t)I2C1->DR;
        if (i == len - 2) {
            i2c1_stop(); /* STOP before reading the second-to-last byte, per ST datasheet */
        }
    }
    if (len == 1) i2c1_stop();
    return 0;
}
