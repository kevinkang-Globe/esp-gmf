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

#include "esp_gmf_data_bus.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Create a new ring buffer with the specified item count and size
 *
 * @param[in]   num       Size of each item
 * @param[in]   item_cnt  Number of items
 * @param[out]  h         Pointer to store the handle of the GMF data bus
 *
 * @return
 *       - 0    On success
 *       - < 0  Negative value if an error occurs
 */
int esp_gmf_db_new_ringbuf(int num, int item_cnt, esp_gmf_db_handle_t *h);

/**
 * @brief  Create a new block buffer with the specified item count and size
 *
 * @param[in]   num       Size of each item
 * @param[in]   item_cnt  Number of items
 * @param[out]  h         Pointer to store the handle of the GMF data bus
 *
 * @return
 *       - 0    On success
 *       - < 0  Negative value if an error occurs
 */
int esp_gmf_db_new_block(int num, int item_cnt, esp_gmf_db_handle_t *h);

/**
 * @brief  Create a new pointer buffer (pbuf) with the specified item count and size
 *
 * @param[in]   num       Size of each item
 * @param[in]   item_cnt  Number of items
 * @param[out]  h         Pointer to store the handle of the GMF data bus
 *
 * @return
 *       - 0    On success
 *       - < 0  Negative value if an error occurs
 */
int esp_gmf_db_new_pbuf(int num, int item_cnt, esp_gmf_db_handle_t *h);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
