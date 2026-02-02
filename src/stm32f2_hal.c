// SPDX-FileCopyrightText: 2026 AmongBytes, Ltd.
// SPDX-FileContributor: Kris Kwiatkowski <kris@amongbytes.com>
// SPDX-License-Identifier: LicenseRef-Proprietary

#include <stddef.h>

#include "stm32f2_hal_lowlevel.h"
#include "stm32f2xx_hal_dma.h"
#include "stm32f2xx_hal_gpio.h"
#include "stm32f2xx_hal_rcc.h"
#include "stm32f2xx_hal_uart.h"

void hal_init(void) {
#ifdef USE_INTERNAL_CLK
    RCC_OscInitTypeDef RCC_OscInitStruct;
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSEState       = RCC_HSE_OFF;
    RCC_OscInitStruct.HSIState       = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLL_NONE;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitTypeDef RCC_ClkInitStruct;
    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                                   RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_HSI;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    uint32_t flash_latency           = 0;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, flash_latency);
#else
    RCC_OscInitTypeDef RCC_OscInitStruct;
    RCC_OscInitStruct.OscillatorType =
        RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSEState      = RCC_HSE_BYPASS;
    RCC_OscInitStruct.HSIState      = RCC_HSI_OFF;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLL_NONE;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitTypeDef RCC_ClkInitStruct;
    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                                   RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_HSE;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    uint32_t flash_latency           = 5;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, flash_latency);
#endif
}
