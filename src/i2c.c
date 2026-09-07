#include "i2c.h"
#include "nvic.h"

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

/* ---- Interrupt-driven read ----
   Same protocol as the blocking i2c1_read() above (CMD sequence: START,
   write reg, repeated START, read N bytes, NACK+STOP before the last byte),
   just split across ISR calls instead of a single blocking function, driven
   by the SB/ADDR/BTF/RXNE events named in RM0090's I2C event descriptions
   (EV5, EV6, EV8_2, EV7). */

typedef enum {
    PHASE_NONE = 0,
    PHASE_START1,   /* waiting for SB after the first START (write direction) */
    PHASE_ADDR1,    /* waiting for ADDR after sending the device address (write) */
    PHASE_REG_SENT, /* waiting for BTF after sending the register byte */
    PHASE_START2,   /* waiting for SB after the repeated START (read direction) */
    PHASE_ADDR2,    /* waiting for ADDR after sending the device address (read) */
    PHASE_RECEIVING,
} i2c_it_phase_t;

static volatile i2c_it_status_t s_it_status = I2C_IT_IDLE;
static volatile i2c_it_phase_t  s_it_phase  = PHASE_NONE;
static uint8_t   s_it_dev_addr;
static uint8_t   s_it_reg;
static uint8_t  *s_it_buf;
static uint16_t  s_it_len;
static uint16_t  s_it_index;

void i2c1_it_init(void)
{
    i2c1_init(); /* reuse the pin mux, clock and CCR/TRISE setup from the polling driver */
    I2C1->CR2 |= I2C_CR2_ITEVFEN | I2C_CR2_ITBUFEN | I2C_CR2_ITERREN;
    nvic_enable_irq(I2C1_EV_IRQn);
    nvic_enable_irq(I2C1_ER_IRQn);
}

i2c_it_status_t i2c1_it_get_status(void)
{
    return s_it_status;
}

i2c_it_status_t i2c1_it_read(uint8_t dev_addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
    if (s_it_status == I2C_IT_BUSY) {
        return I2C_IT_ERROR; /* a transaction is already in flight, refuse to start another */
    }

    s_it_dev_addr = dev_addr;
    s_it_reg      = reg;
    s_it_buf      = buf;
    s_it_len      = len;
    s_it_index    = 0;
    s_it_phase    = PHASE_START1;
    s_it_status   = I2C_IT_BUSY;

    I2C1->CR1 |= I2C_CR1_ACK;
    I2C1->CR1 |= I2C_CR1_START; /* triggers the SB event -> I2C1_EV_IRQHandler */
    return I2C_IT_BUSY;
}

void I2C1_EV_IRQHandler(void)
{
    uint32_t sr1 = I2C1->SR1;

    switch (s_it_phase) {
    case PHASE_START1:
        if (sr1 & I2C_SR1_SB) {
            I2C1->DR = (uint8_t)(s_it_dev_addr << 1); /* address + write bit */
            s_it_phase = PHASE_ADDR1;
        }
        break;

    case PHASE_ADDR1:
        if (sr1 & I2C_SR1_ADDR) {
            (void)I2C1->SR1;
            (void)I2C1->SR2; /* clear ADDR by reading SR1 then SR2 */
            I2C1->DR = s_it_reg; /* send the register address */
            s_it_phase = PHASE_REG_SENT;
        }
        break;

    case PHASE_REG_SENT:
        if (sr1 & I2C_SR1_BTF) {
            I2C1->CR1 |= I2C_CR1_START; /* repeated START, triggers SB again */
            s_it_phase = PHASE_START2;
        }
        break;

    case PHASE_START2:
        if (sr1 & I2C_SR1_SB) {
            if (s_it_len == 1) {
                I2C1->CR1 &= ~I2C_CR1_ACK; /* NACK the only byte, per RM0090's single-byte read sequence */
            }
            I2C1->DR = (uint8_t)((s_it_dev_addr << 1) | 1); /* address + read bit */
            s_it_phase = PHASE_ADDR2;
        }
        break;

    case PHASE_ADDR2:
        if (sr1 & I2C_SR1_ADDR) {
            (void)I2C1->SR1;
            (void)I2C1->SR2; /* clear ADDR */
            if (s_it_len == 1) {
                I2C1->CR1 |= I2C_CR1_STOP; /* STOP right after clearing ADDR, single-byte case */
            }
            s_it_phase = PHASE_RECEIVING;
        }
        break;

    case PHASE_RECEIVING:
        if (sr1 & I2C_SR1_RXNE) {
            if (s_it_len >= 2 && s_it_index == s_it_len - 2) {
                I2C1->CR1 &= ~I2C_CR1_ACK; /* NACK the last byte */
                I2C1->CR1 |= I2C_CR1_STOP; /* STOP before reading the second-to-last byte */
            }
            s_it_buf[s_it_index++] = (uint8_t)I2C1->DR;
            if (s_it_index >= s_it_len) {
                s_it_phase  = PHASE_NONE;
                s_it_status = I2C_IT_DONE;
            }
        }
        break;

    default:
        break; /* stray event with no transaction in flight; nothing to do */
    }
}

void I2C1_ER_IRQHandler(void)
{
    uint32_t sr1 = I2C1->SR1;

    if (sr1 & (I2C_SR1_AF | I2C_SR1_BERR | I2C_SR1_ARLO | I2C_SR1_OVR)) {
        I2C1->SR1 &= ~(I2C_SR1_AF | I2C_SR1_BERR | I2C_SR1_ARLO | I2C_SR1_OVR); /* clear error flags */
        I2C1->CR1 |= I2C_CR1_STOP;
        s_it_phase  = PHASE_NONE;
        s_it_status = I2C_IT_ERROR;
    }
}
