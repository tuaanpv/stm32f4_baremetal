/**
 * startup_stm32f407xx.s
 * Minimal startup code for STM32F407VG (Cortex-M4)
 * Vector table + Reset_Handler + weak default handlers
 */

    .syntax unified
    .cpu cortex-m4
    .fpu softvfp
    .thumb

.global g_pfnVectors
.global Default_Handler

/* Symbols from linker script */
.word _sidata
.word _sdata
.word _edata
.word _sbss
.word _ebss

    .section .text.Reset_Handler
    .weak Reset_Handler
    .type Reset_Handler, %function
Reset_Handler:
    /* Copy .data section from FLASH to RAM */
    ldr r0, =_sdata
    ldr r1, =_edata
    ldr r2, =_sidata
    movs r3, #0
    b LoopCopyDataInit

CopyDataInit:
    ldr r4, [r2, r3]
    str r4, [r0, r3]
    adds r3, r3, #4

LoopCopyDataInit:
    adds r4, r0, r3
    cmp r4, r1
    bcc CopyDataInit

    /* Zero-fill .bss section */
    ldr r2, =_sbss
    ldr r4, =_ebss
    movs r3, #0
    b LoopFillZerobss

FillZerobss:
    str  r3, [r2]
    adds r2, r2, #4

LoopFillZerobss:
    cmp r2, r4
    bcc FillZerobss

    /* Call main */
    bl main
    bx lr
.size Reset_Handler, .-Reset_Handler

/* Default handler for unhandled interrupts */
    .section .text.Default_Handler,"ax",%progbits
Default_Handler:
Infinite_Loop:
    b Infinite_Loop
    .size Default_Handler, .-Default_Handler

/**
 * Vector table. SP/Reset/NMI/HardFault/... core exceptions are set explicitly.
 * External IRQs (IRQ0..IRQ81) point at Default_Handler by default so an
 * unexpected interrupt just loops instead of jumping into garbage memory --
 * except I2C1_EV (31), I2C1_ER (32) and USART2 (38), which have named,
 * weakly-aliased handlers so uart.c/i2c.c can override them with real ISRs.
 */
    .section .isr_vector,"a",%progbits
    .type g_pfnVectors, %object
    .size g_pfnVectors, .-g_pfnVectors

g_pfnVectors:
    .word _estack
    .word Reset_Handler
    .word NMI_Handler
    .word HardFault_Handler
    .word MemManage_Handler
    .word BusFault_Handler
    .word UsageFault_Handler
    .word 0
    .word 0
    .word 0
    .word 0
    .word SVC_Handler
    .word DebugMon_Handler
    .word 0
    .word PendSV_Handler
    .word SysTick_Handler
    /* IRQ0..IRQ30 (31 entries): not used in this project */
    .rept 31
    .word Default_Handler
    .endr
    .word I2C1_EV_IRQHandler   /* IRQ31 */
    .word I2C1_ER_IRQHandler   /* IRQ32 */
    /* IRQ33..IRQ37 (5 entries: I2C2_EV, I2C2_ER, SPI1, SPI2, USART1): not used */
    .rept 5
    .word Default_Handler
    .endr
    .word USART2_IRQHandler    /* IRQ38 */
    /* IRQ39..IRQ81 (43 entries): not used in this project */
    .rept 43
    .word Default_Handler
    .endr

    .weak NMI_Handler
    .thumb_set NMI_Handler,Default_Handler
    .weak HardFault_Handler
    .thumb_set HardFault_Handler,Default_Handler
    .weak MemManage_Handler
    .thumb_set MemManage_Handler,Default_Handler
    .weak BusFault_Handler
    .thumb_set BusFault_Handler,Default_Handler
    .weak UsageFault_Handler
    .thumb_set UsageFault_Handler,Default_Handler
    .weak SVC_Handler
    .thumb_set SVC_Handler,Default_Handler
    .weak DebugMon_Handler
    .thumb_set DebugMon_Handler,Default_Handler
    .weak PendSV_Handler
    .thumb_set PendSV_Handler,Default_Handler
    .weak SysTick_Handler
    .thumb_set SysTick_Handler,Default_Handler
    .weak I2C1_EV_IRQHandler
    .thumb_set I2C1_EV_IRQHandler,Default_Handler
    .weak I2C1_ER_IRQHandler
    .thumb_set I2C1_ER_IRQHandler,Default_Handler
    .weak USART2_IRQHandler
    .thumb_set USART2_IRQHandler,Default_Handler

