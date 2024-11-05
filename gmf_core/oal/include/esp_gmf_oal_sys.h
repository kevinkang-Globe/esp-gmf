/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2024 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once

#include "esp_gmf_err.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#if defined(__GNUC__) && (__GNUC__ >= 7)
#define FALL_THROUGH __attribute__((fallthrough))
#else
#define FALL_THROUGH ((void)0)
#endif  /* defined(__GNUC__) && (__GNUC__ >= 7) */

/**
 * @brief  Get system ticks based on the given millisecond value
 *
 * @param[in]  ms  Time in milliseconds
 *
 * @return
 *       - The  corresponding system ticks
 */
int esp_gmf_oal_sys_get_tick_by_time_ms(int ms);

/**
 * @brief  Retrieve the current system time in milliseconds
 *
 * @return
 *       - The  system time in milliseconds
 */
int64_t esp_gmf_oal_sys_get_time_ms(void);

/**
 * @brief  Print CPU usage statistics of tasks over a specified time period
 *
 *         This function measures and prints the CPU usage of tasks over a given
 *         period (in milliseconds) by calling `uxTaskGetSystemState()` twice,
 *         with a delay in between. The CPU usage is then calculated based on the
 *         difference in task run times before and after the delay.
 *
 * @note
 *        - If tasks are added or removed during the delay, their statistics will not be included in the final report
 *        - To minimize inaccuracies caused by delays, this function should be called from a high-priority task
 *        - In dual-core mode, each core will account for up to 50% of the total runtime
 *
 * @param[in]  elapsed_time_ms  Time period for the CPU usage measurement in milliseconds.
 *
 * @return
 *       - ESP_GMF_ERR_OK           On success
 *       - ESP_GMF_ERR_MEMORY_LACK  No memory for operation
 *       - ESP_GMF_ERR_NOT_ENOUGH   More memory is needed for uxTaskGetSystemState
 *       - ESP_GMF_ERR_FAIL         On failure
 */
esp_gmf_err_t esp_gmf_oal_sys_get_real_time_stats(int elapsed_time_ms);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
