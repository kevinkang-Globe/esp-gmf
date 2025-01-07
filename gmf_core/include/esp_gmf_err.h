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

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define ESP_GMF_ERR_BASE      (0x60000)
#define ESP_GMF_ERR_CORE_BASE (ESP_GMF_ERR_BASE + 0x0)

/**
 * @brief  Error codes for GMF IO operations
 */
typedef enum {
    ESP_GMF_IO_OK      = ESP_OK,    /*!< Operation successful */
    ESP_GMF_IO_FAIL    = ESP_FAIL,  /*!< Operation failed */
    ESP_GMF_IO_ABORT   = -2,        /*!< Operation aborted */
    ESP_GMF_IO_TIMEOUT = -3,        /*!< Operation timed out */
} esp_gmf_err_io_t;

typedef enum {
    ESP_GMF_ERR_OK             = ESP_OK,
    ESP_GMF_ERR_FAIL           = ESP_FAIL,
    ESP_GMF_ERR_UNKNOWN        = ESP_GMF_ERR_CORE_BASE + 0,
    ESP_GMF_ERR_ALREADY_EXISTS = ESP_GMF_ERR_CORE_BASE + 1,
    ESP_GMF_ERR_MEMORY_LACK    = ESP_GMF_ERR_CORE_BASE + 2,
    ESP_GMF_ERR_INVALID_URI    = ESP_GMF_ERR_CORE_BASE + 3,
    ESP_GMF_ERR_INVALID_PATH   = ESP_GMF_ERR_CORE_BASE + 4,
    ESP_GMF_ERR_INVALID_ARG    = ESP_GMF_ERR_CORE_BASE + 5,
    ESP_GMF_ERR_INVALID_STATE  = ESP_GMF_ERR_CORE_BASE + 6,
    ESP_GMF_ERR_OUT_OF_RANGE   = ESP_GMF_ERR_CORE_BASE + 7,
    ESP_GMF_ERR_NOT_READY      = ESP_GMF_ERR_CORE_BASE + 8,
    ESP_GMF_ERR_NOT_SUPPORT    = ESP_GMF_ERR_CORE_BASE + 9,
    ESP_GMF_ERR_NOT_FOUND      = ESP_GMF_ERR_CORE_BASE + 10,
    ESP_GMF_ERR_NOT_ENOUGH     = ESP_GMF_ERR_CORE_BASE + 12,
    ESP_GMF_ERR_NO_DATA        = ESP_GMF_ERR_CORE_BASE + 13,
    ESP_GMF_ERR_TIMEOUT        = ESP_GMF_ERR_CORE_BASE + 14,
} esp_gmf_err_t;

#define ESP_GMF_CHECK(TAG, a, action, msg) if (!(a)) {                           \
    ESP_LOGE(TAG, "%s:%d (%s): %s", __FILENAME__, __LINE__, __FUNCTION__, msg);  \
    action;                                                                      \
}

#define ESP_GMF_RET_ON_FAIL(TAG, a, action, msg) if (unlikely(a == ESP_FAIL)) {  \
    ESP_LOGE(TAG, "%s:%d (%s): %s", __FILENAME__, __LINE__, __FUNCTION__, msg);  \
    action;                                                                      \
}

#define ESP_GMF_RET_ON_NOT_OK(TAG, a, action, msg) if (unlikely(a != ESP_OK)) {  \
    ESP_LOGE(TAG, "%s:%d (%s): %s", __FILENAME__, __LINE__, __FUNCTION__, msg);  \
    action;                                                                      \
}

#define ESP_GMF_RET_ON_ERROR(log_tag, a, action, format, ...) do {                                \
    esp_err_t err_rc_ = (a);                                                                      \
    if (unlikely(err_rc_ != ESP_OK)) {                                                            \
        ESP_LOGE(log_tag, "%s(%d): " format, __FUNCTION__, __LINE__ __VA_OPT__(, ) __VA_ARGS__);  \
        action;                                                                                   \
    }                                                                                             \
} while (0)

#define ESP_GMF_MEM_VERIFY(TAG, a, action, name, size) if (!(a)) {                                      \
    ESP_LOGE(TAG, "%s(%d): Failed to allocate memory for %s(%d).", __FUNCTION__, __LINE__, name, size); \
    action;                                                                                             \
}

#define ESP_GMF_MEM_CHECK(TAG, a, action) ESP_GMF_CHECK(TAG, a, action, "Memory exhausted")

#define ESP_GMF_NULL_CHECK(TAG, a, action) ESP_GMF_CHECK(TAG, a, action, "Got NULL Pointer")

#ifdef __cplusplus
}
#endif  /* __cplusplus */
