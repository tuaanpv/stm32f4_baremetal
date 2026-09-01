#include "stm32f4xx.h"
#include "gpio.h"
#include "delay.h"
#include "adc.h"
#include "i2c.h"
#include "uart.h"
#include "pwm.h"
#include "spi.h"
#include "sdcard.h"
#include "ff.h"

/* Example: change to the real 7-bit address of your I2C sensor (e.g. MPU6050 = 0x68) */
#define SENSOR_I2C_ADDR   0x68
#define SENSOR_WHO_AM_I_REG 0x75

int main(void)
{
    /* Note: this project runs on the default HSI (16MHz), PLL not configured up to 168MHz.
       That's enough for the GPIO/ADC/I2C/UART/PWM demo. For full speed, add a step to
       configure RCC->PLLCFGR and switch SW to PLL in CFGR before calling the other
       init functions (and recompute the UART/PWM clock arguments below to match). */

    delay_init(16000000UL); /* SysTick runs off HSI 16MHz */
    led_init();
    adc1_init();
    i2c1_init();
    uart2_init(16000000UL, 115200UL);      /* USART2: PA2=TX, PA3=RX, 115200 8N1 */
    pwm_tim3_ch1_init(16000000UL, 1000UL); /* TIM3 CH1 on PA6, 1kHz PWM */

    uart2_write_string("STM32F407 bare-metal demo booting...\r\n");

    /* SD card over SPI1 (SCK=PB3, MISO=PB4, MOSI=PB5, CS=PA4).
       disk_initialize() inside diskio.c calls spi1_init()/sd_init() for us,
       so f_mount() is enough to bring the card + filesystem online. */
    static FATFS fs;
    static FIL log_file;
    FRESULT fr = f_mount(&fs, "", 1); /* mount immediately (last arg = 1) to catch errors here */

    if (fr == FR_OK) {
        uart2_write_string("SD card + FAT filesystem mounted OK\r\n");
    } else {
        uart2_write_string("f_mount failed, FRESULT=");
        uart2_write_uint((uint32_t)fr);
        uart2_write_string("\r\n");
    }

    /* Open (or create) LOG.TXT and keep it open for the whole session,
       appending one line per ADC sample; f_sync() after each write
       flushes to the card without closing the file. */
    int log_ok = 0;
    if (fr == FR_OK) {
        fr = f_open(&log_file, "LOG.TXT", FA_WRITE | FA_OPEN_APPEND);
        log_ok = (fr == FR_OK);
        if (!log_ok) {
            uart2_write_string("f_open(LOG.TXT) failed, FRESULT=");
            uart2_write_uint((uint32_t)fr);
            uart2_write_string("\r\n");
        }
    }

    uint8_t who_am_i = 0;
    /* Try reading a sensor identification register to check whether I2C is alive */
    int i2c_ok = (i2c1_read(SENSOR_I2C_ADDR, SENSOR_WHO_AM_I_REG, &who_am_i, 1) == 0);

    uint16_t duty = 0;
    int8_t duty_step = 50; /* permille per loop -> ramps 0..1000 and back for a "breathing" effect */

    while (1) {
        led_toggle(LED_GREEN_PIN);

        uint16_t adc_val = adc1_read(); /* 0..4095, reads the voltage on PA0 */

        if (i2c_ok) {
            led_on(LED_BLUE_PIN);   /* indicates I2C communication was OK at startup */
        } else {
            led_on(LED_RED_PIN);    /* indicates I2C failed / device not found */
        }

        /* Example: write 1 config byte to register 0x6B (PWR_MGMT_1 on the MPU6050) */
        uint8_t cfg = 0x00;
        i2c1_write(SENSOR_I2C_ADDR, 0x6B, &cfg, 1);

        /* Report the ADC reading over UART */
        uart2_write_string("ADC=");
        uart2_write_uint(adc_val);
        uart2_write_string(" I2C=");
        uart2_write_string(i2c_ok ? "OK" : "FAIL");
        uart2_write_string(" DUTY=");
        uart2_write_uint(duty);
        uart2_write_string("\r\n");

        /* Append this sample to LOG.TXT on the SD card, then flush without closing */
        if (log_ok) {
            f_printf(&log_file, "ADC=%u I2C=%s DUTY=%u\n",
                      (unsigned)adc_val, i2c_ok ? "OK" : "FAIL", (unsigned)duty);
            f_sync(&log_file);
        }

        /* Ramp the PWM duty cycle up and down (LED brightness on PA6, external LED) */
        pwm_tim3_ch1_set_duty(duty);
        duty += duty_step;
        if (duty >= 1000) { duty = 1000; duty_step = -duty_step; }
        if (duty == 0)     { duty_step = -duty_step; }

        delay_ms(100);
    }
}

