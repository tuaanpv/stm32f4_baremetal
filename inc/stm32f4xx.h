/**
 * stm32f4xx.h
 * Minimal bare-metal register definitions for STM32F407xx
 * (Custom-written, CMSIS-style, only covers the peripherals used in this
 *  project: RCC, GPIO, ADC1, I2C1, basic SysTick/NVIC)
 *
 * NOT the official ST CMSIS file — just enough for this learning example.
 * For a production project, use the official ST CMSIS + HAL/LL instead.
 */

#ifndef __STM32F4XX_H
#define __STM32F4XX_H

#include <stdint.h>

/* ---------------- Core / Memory map ---------------- */
#define PERIPH_BASE           0x40000000UL
#define APB1PERIPH_BASE        (PERIPH_BASE)
#define APB2PERIPH_BASE        (PERIPH_BASE + 0x00010000UL)
#define AHB1PERIPH_BASE         (PERIPH_BASE + 0x00020000UL)

#define RCC_BASE               (AHB1PERIPH_BASE + 0x3800UL)
#define GPIOA_BASE              (AHB1PERIPH_BASE + 0x0000UL)
#define GPIOB_BASE               (AHB1PERIPH_BASE + 0x0400UL)
#define GPIOC_BASE                (AHB1PERIPH_BASE + 0x0800UL)
#define GPIOD_BASE                 (AHB1PERIPH_BASE + 0x0C00UL)

#define ADC1_BASE               (APB2PERIPH_BASE + 0x2000UL)
#define I2C1_BASE                (APB1PERIPH_BASE + 0x5400UL)
#define USART2_BASE               (APB1PERIPH_BASE + 0x4400UL)
#define TIM3_BASE                  (APB1PERIPH_BASE + 0x0400UL)
#define SPI1_BASE                   (APB2PERIPH_BASE + 0x3000UL)

/* ---------------- RCC ---------------- */
typedef struct {
    volatile uint32_t CR;
    volatile uint32_t PLLCFGR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
    volatile uint32_t AHB1RSTR;
    volatile uint32_t AHB2RSTR;
    volatile uint32_t AHB3RSTR;
    uint32_t RESERVED0;
    volatile uint32_t APB1RSTR;
    volatile uint32_t APB2RSTR;
    uint32_t RESERVED1[2];
    volatile uint32_t AHB1ENR;
    volatile uint32_t AHB2ENR;
    volatile uint32_t AHB3ENR;
    uint32_t RESERVED2;
    volatile uint32_t APB1ENR;
    volatile uint32_t APB2ENR;
    uint32_t RESERVED3[2];
    volatile uint32_t AHB1LPENR;
    volatile uint32_t AHB2LPENR;
    volatile uint32_t AHB3LPENR;
    uint32_t RESERVED4;
    volatile uint32_t APB1LPENR;
    volatile uint32_t APB2LPENR;
    uint32_t RESERVED5[2];
    volatile uint32_t BDCR;
    volatile uint32_t CSR;
    uint32_t RESERVED6[2];
    volatile uint32_t SSCGR;
    volatile uint32_t PLLI2SCFGR;
} RCC_TypeDef;

#define RCC     ((RCC_TypeDef *) RCC_BASE)

/* RCC_AHB1ENR bits */
#define RCC_AHB1ENR_GPIOAEN     (1UL << 0)
#define RCC_AHB1ENR_GPIOBEN     (1UL << 1)
#define RCC_AHB1ENR_GPIOCEN     (1UL << 2)
#define RCC_AHB1ENR_GPIODEN     (1UL << 3)

/* RCC_APB2ENR bits */
#define RCC_APB2ENR_ADC1EN      (1UL << 8)
#define RCC_APB2ENR_SPI1EN      (1UL << 12)

/* RCC_APB1ENR bits */
#define RCC_APB1ENR_TIM3EN      (1UL << 1)
#define RCC_APB1ENR_I2C1EN      (1UL << 21)
#define RCC_APB1ENR_USART2EN    (1UL << 17)

/* ---------------- GPIO ---------------- */
typedef struct {
    volatile uint32_t MODER;
    volatile uint32_t OTYPER;
    volatile uint32_t OSPEEDR;
    volatile uint32_t PUPDR;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t LCKR;
    volatile uint32_t AFR[2]; /* AFR[0]=AFRL, AFR[1]=AFRH */
} GPIO_TypeDef;

#define GPIOA   ((GPIO_TypeDef *) GPIOA_BASE)
#define GPIOB   ((GPIO_TypeDef *) GPIOB_BASE)
#define GPIOC   ((GPIO_TypeDef *) GPIOC_BASE)
#define GPIOD   ((GPIO_TypeDef *) GPIOD_BASE)

#define GPIO_MODE_INPUT     0x0UL
#define GPIO_MODE_OUTPUT    0x1UL
#define GPIO_MODE_AF        0x2UL
#define GPIO_MODE_ANALOG    0x3UL

/* ---------------- ADC1 ---------------- */
typedef struct {
    volatile uint32_t SR;
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t SMPR1;
    volatile uint32_t SMPR2;
    volatile uint32_t JOFR[4];
    volatile uint32_t HTR;
    volatile uint32_t LTR;
    volatile uint32_t SQR1;
    volatile uint32_t SQR2;
    volatile uint32_t SQR3;
    volatile uint32_t JSQR;
    volatile uint32_t JDR[4];
    volatile uint32_t DR;
} ADC_TypeDef;

#define ADC1    ((ADC_TypeDef *) ADC1_BASE)

#define ADC_CR2_ADON        (1UL << 0)
#define ADC_CR2_SWSTART     (1UL << 30)
#define ADC_SR_EOC          (1UL << 1)

/* ---------------- I2C1 ---------------- */
typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t OAR1;
    volatile uint32_t OAR2;
    volatile uint32_t DR;
    volatile uint32_t SR1;
    volatile uint32_t SR2;
    volatile uint32_t CCR;
    volatile uint32_t TRISE;
    volatile uint32_t FLTR;
} I2C_TypeDef;

#define I2C1    ((I2C_TypeDef *) I2C1_BASE)

#define I2C_CR1_PE          (1UL << 0)
#define I2C_CR1_START       (1UL << 8)
#define I2C_CR1_STOP        (1UL << 9)
#define I2C_CR1_ACK         (1UL << 10)
#define I2C_SR1_SB          (1UL << 0)
#define I2C_SR1_ADDR        (1UL << 1)
#define I2C_SR1_BTF         (1UL << 2)
#define I2C_SR1_RXNE        (1UL << 6)
#define I2C_SR1_TXE         (1UL << 7)

/* ---------------- USART2 ---------------- */
typedef struct {
    volatile uint32_t SR;
    volatile uint32_t DR;
    volatile uint32_t BRR;
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t CR3;
    volatile uint32_t GTPR;
} USART_TypeDef;

#define USART2  ((USART_TypeDef *) USART2_BASE)

#define USART_SR_TXE        (1UL << 7)
#define USART_SR_TC         (1UL << 6)
#define USART_SR_RXNE        (1UL << 5)
#define USART_CR1_RE         (1UL << 2)
#define USART_CR1_TE         (1UL << 3)
#define USART_CR1_UE         (1UL << 13)

/* ---------------- TIM3 (general-purpose timer, used for PWM here) ---------------- */
typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t SMCR;
    volatile uint32_t DIER;
    volatile uint32_t SR;
    volatile uint32_t EGR;
    volatile uint32_t CCMR1;
    volatile uint32_t CCMR2;
    volatile uint32_t CCER;
    volatile uint32_t CNT;
    volatile uint32_t PSC;
    volatile uint32_t ARR;
    uint32_t RESERVED0;
    volatile uint32_t CCR1;
    volatile uint32_t CCR2;
    volatile uint32_t CCR3;
    volatile uint32_t CCR4;
    uint32_t RESERVED1;
    volatile uint32_t DCR;
    volatile uint32_t DMAR;
} TIM_TypeDef;

#define TIM3    ((TIM_TypeDef *) TIM3_BASE)

#define TIM_CR1_CEN          (1UL << 0)
#define TIM_CR1_ARPE         (1UL << 7)
#define TIM_CCMR1_OC1PE      (1UL << 3)
#define TIM_CCMR1_OC1M_PWM1  (0x6UL << 4) /* PWM mode 1 */
#define TIM_CCER_CC1E        (1UL << 0)

/* ---------------- SPI1 ---------------- */
typedef struct {
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t SR;
    volatile uint32_t DR;
    volatile uint32_t CRCPR;
    volatile uint32_t RXCRCR;
    volatile uint32_t TXCRCR;
    volatile uint32_t I2SCFGR;
    volatile uint32_t I2SPR;
} SPI_TypeDef;

#define SPI1    ((SPI_TypeDef *) SPI1_BASE)

#define SPI_CR1_CPHA         (1UL << 0)
#define SPI_CR1_CPOL         (1UL << 1)
#define SPI_CR1_MSTR         (1UL << 2)
#define SPI_CR1_BR_DIV2      (0x0UL << 3)
#define SPI_CR1_BR_DIV4      (0x1UL << 3)
#define SPI_CR1_BR_DIV8      (0x2UL << 3)
#define SPI_CR1_BR_DIV256    (0x7UL << 3)
#define SPI_CR1_SPE          (1UL << 6)
#define SPI_CR1_SSI          (1UL << 8)
#define SPI_CR1_SSM          (1UL << 9)
#define SPI_SR_RXNE          (1UL << 0)
#define SPI_SR_TXE           (1UL << 1)
#define SPI_SR_BSY           (1UL << 7)

/* ---------------- SysTick (ARM Cortex-M core peripheral) ---------------- */
typedef struct {
    volatile uint32_t CTRL;
    volatile uint32_t LOAD;
    volatile uint32_t VAL;
    volatile uint32_t CALIB;
} SysTick_TypeDef;

#define SysTick_BASE    0xE000E010UL
#define SysTick         ((SysTick_TypeDef *) SysTick_BASE)

#define SysTick_CTRL_ENABLE    (1UL << 0)
#define SysTick_CTRL_TICKINT   (1UL << 1)
#define SysTick_CTRL_CLKSOURCE (1UL << 2)
#define SysTick_CTRL_COUNTFLAG (1UL << 16)

#endif /* __STM32F4XX_H */
