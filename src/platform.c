// SPDX-FileCopyrightText: 2026 AmongBytes, Ltd.
// SPDX-FileContributor: Kris Kwiatkowski <kris@amongbytes.com>
// SPDX-License-Identifier: LicenseRef-Proprietary

#include <platform/platform.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// clang-format off
#include "printf.h"
#include "stm32f2xx_hal_dma.h"

#include "stm32f2xx_hal_gpio.h"
#include "stm32f2xx_hal_gpio_ex.h"
#include "stm32f2xx_hal_uart.h"
#include "stm32f2xx_hal_rcc.h"
// clang-format on

// UART handle
static UART_HandleTypeDef UartHandle;
// Cycle count support
static bool cycle_cnt_initialized = false;
/* SysTick overflow counter, used to measure time in milliseconds since
 * platform initialization. It is incremented every 1 ms by
 * SysTick_Handler. It can be used to measure for up to to 49.7 days
 * (2^32 / 24 * 60 * 60 * 1000). */
volatile static uint32_t k_systick_milliseconds = 0;

/// ############################
/// Internal forward declarations
/// ############################
void     hal_init(void);
void     setup_rng(void);
uint32_t xorshift32_u32(void);

/// ############################
/// UART Initialization
/// ############################

static void uart1_init(UART_HandleTypeDef *huart, size_t baudrate) {
    GPIO_InitTypeDef GpioInit;
    GpioInit.Pin       = GPIO_PIN_9 | GPIO_PIN_10;
    GpioInit.Mode      = GPIO_MODE_AF_PP;
    GpioInit.Pull      = GPIO_PULLUP;
    GpioInit.Speed     = GPIO_SPEED_FREQ_HIGH;
    GpioInit.Alternate = GPIO_AF7_USART1;
    __GPIOA_CLK_ENABLE();
    HAL_GPIO_Init(GPIOA, &GpioInit);

    huart->Instance        = USART1;
    huart->Init.BaudRate   = baudrate;
    huart->Init.WordLength = UART_WORDLENGTH_8B;
    huart->Init.StopBits   = UART_STOPBITS_1;
    huart->Init.Parity     = UART_PARITY_NONE;
    huart->Init.HwFlowCtl  = UART_HWCONTROL_NONE;
    huart->Init.Mode       = UART_MODE_TX_RX;
    __USART1_CLK_ENABLE();
    HAL_UART_Init(huart);
}

// SysTick handler
void SysTick_Handler(void) { ++k_systick_milliseconds; }

/// ############################
/// I/O
/// ############################

char platform_getch(void) {
    uint8_t d;
    while (HAL_UART_Receive(&UartHandle, &d, 1, 5000) != HAL_OK)
        ;
    return d;
}

// Used by printf
void platform_putchar(char c) {
    uint8_t d = c;
    HAL_UART_Transmit(&UartHandle, &d, 1, 5000);
}

void _putchar(char character) { platform_putchar(character); }

/// ############################
/// Cycle count support
/// ############################

static void cyclecount_reset(void) {
    // This is valid approach according to ARM DDI 0403E.b, section C1.8.8.
    DWT->CYCCNT = 0;  // 0 is a reset value.
}

/* @brief Enables CPU cycle counting.
 *
 * This implementation uses Debug Watchpoint and Trace (DWT) unit's.
 * According to SmartFusion2 data sheet, the Cortex-M3 core implements DWT as
 * per ARMv7-M architecture reference manual (ARM DDI 0403E.b). That means, we
 * can follow the procedure described in ARM DDI 0403E.b, but we must avoid all
 * implementation-defined features, as there is no documentation about them.
 *
 * According to ARM DDI 0403E.b:
 * - To check if DWT is present, we can follow Table C1-2.
 * - To enable CYCCNT, we need to set TRCENA in DEMCR and CYCCNTENA in DWT_CTRL.
 * - To read the cycle count, we can read DWT_CYCCNT.
 * - To reset the cycle count, we can write 0 to DWT_CYCCNT (I assume this is
 * the only allowed reset value).
 *
 * This function enables Trace Enable bit in the Debug Exception and Monitor
 * Control Register. This bit is used to enable several features, including DWT.
 * According to ARM DDI 0403E.b, section C1.6.5, this functionality can only be
 * DISABLED after all ITM and DWT features are disabled. As it would be hard to
 * track that in a generic way, we never disable TRCENA after it is enabled, to
 * make sure results returned by DWT are reliable
 *
 * @retval true if cycle counting was successfully enabled either by the call
 *         to this function or in the past.
 * @retval false if cycle counting could not be enabled (e.g., DWT not present).
 */
static bool cyclecount_init(void) {
    if (CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk) {
        return true;  // Already initialized
    }

    /* Performs a DWT availability check, according to Table C1-2. Note that
     * this check can be done before enabling TRCENA. */
    if (!DWT) {
        return false;  // DWT not present
    }

    /* Enable. It must be done before any DWT registers access (see ARM DDI
     * 0403E.b, section C1.6.5).  */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    // Check if CYCCNT is present. This must be done after enabling TRCENA.
    if (DWT->CTRL & DWT_CTRL_NOCYCCNT_Msk) {
        return false;  // CYCCNT not present
    }

    // Reset cycle counter
    DWT->CYCCNT = 0;

    /* Start cycle counter. Note that this operation will try to write read-only
     * bits 31:28 (NUMCOMP). Nevertheless, Writing 0 does not affect them; they
     * will still read back as the hardware value. */
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    // Wait until cycle counter is enabled
    while (!(DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk)) {
    }
    // Wait until cycle counter starts counting
    while (DWT->CYCCNT == 0) {
    }

    return true;
}

/// ############################
/// Trigger pin for SCA. Compatible with ChipWhisperer
/// ############################
void trigger_init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GpioInit;
    GpioInit.Pin   = GPIO_PIN_12;
    GpioInit.Mode  = GPIO_MODE_OUTPUT_PP;
    GpioInit.Pull  = GPIO_NOPULL;
    GpioInit.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GpioInit);

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, RESET);
}

// Trigger pin control for SCA high
static void trigger_high(void) { HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, SET); }

// Trigger pin control for SCA low
static void trigger_low(void) { HAL_GPIO_WritePin(GPIOA, GPIO_PIN_12, RESET); }

//static void setup_rng(void) { rng_enable(); }

/// ############################
/// External API
/// ############################

int platform_init(platform_op_mode_t a) {
    hal_init();
    cyclecount_init();
    /*
    #if SS_VER == SS_VER_2_1
    uart1_init(&UartHandle, 230400);
#else
    uart1_init(&UartHandle, 38400);
#endif
    */
    uart1_init(&UartHandle, 230400);
    trigger_init();
    setup_rng();
    return 0;
}

bool platform_set_attr(const struct platform_attr_t *a) {
    size_t i;
    for (i = 0; i < a->n; i++) {
        switch (a->attr[i]) {
            case PLATFORM_CLOCK_USERSPACE:
            case PLATFORM_CLOCK_MAX:
                break;
            case PLATFORM_SCA_TRIGGER_HIGH:
                trigger_high();
                return true;
            case PLATFORM_SCA_TRIGGER_LOW:
                trigger_low();
                return true;
            default:
                break;
        }
    }
    return false;
}

uint64_t platform_cpu_cyclecount(void) {
    /* Note that CYCCNT is a 32-bit counter that wraps around. Here we are
     * casting to uint64_t to match the function signature, but the value is still
     * 32-bit and will wrap around every ~4295 million cycles.
     * Should better precision be needed, one can adapt SysTick. */
    return DWT->CYCCNT;
}

void platform_sync(void) {  // No-op on this platform.
}

int platform_get_random(void *out, unsigned len) {
    union {
        unsigned char aschar[4];
        uint32_t      asint;
    } random;

    uint8_t *obuf = (uint8_t *)out;

    while (len > 4) {
        random.asint = xorshift32_u32();
        *obuf++      = random.aschar[0];
        *obuf++      = random.aschar[1];
        *obuf++      = random.aschar[2];
        *obuf++      = random.aschar[3];
        len          -= 4;
    }
    if (len > 0) {
        for (random.asint = xorshift32_u32(); len > 0; --len) {
            *obuf++ = random.aschar[len - 1];
        }
    }

    return 0;
}
