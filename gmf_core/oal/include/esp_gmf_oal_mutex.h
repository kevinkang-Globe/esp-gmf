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

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Allocates and initializes a new mutex object for synchronization
 *
 * @return
 *       - Pointer  to the newly created mutex on success
 *       - NULL     if the mutex creation fails
 */
void *esp_gmf_oal_mutex_create(void);

/**
 * @brief  Destroy a mutex
 *
 * @param[in]  mutex  Pointer to the mutex to destroy
 *
 * @return
 *       - 0         on success
 *       - Negative  value if an error occurs
 */
int esp_gmf_oal_mutex_destroy(void *mutex);

/**
 * @brief  Acquires a lock on the specified mutex, blocking if necessary until the lock becomes available
 *
 * @param[in]  mutex  Pointer to the mutex to lock
 *
 * @return
 *       - 0         on success
 *       - Negative  value if an error occurs
 */
int esp_gmf_oal_mutex_lock(void *mutex);

/**
 * @brief  Releases the lock held on the specified mutex, allowing other threads to acquire the lock
 *
 * @param[in]  mutex  Pointer to the mutex to unlock
 *
 * @return
 *       - 0         on success
 *       - Negative  value if an error occurs
 */
int esp_gmf_oal_mutex_unlock(void *mutex);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
