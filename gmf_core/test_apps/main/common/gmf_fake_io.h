/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2024 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD.>
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

#include "esp_gmf_io.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Fake IO configurations
 */
typedef struct {
    int          dir;   /*!< IO direction, reader or writer */
    const char  *name;  /*!< Name for this instance */
} fake_io_cfg_t;

#define FAKE_IO_CFG_DEFAULT() {   \
    .dir  = ESP_GMF_IO_DIR_NONE,  \
    .name = NULL,                 \
}

/**
 * @brief  Initializes the fake stream I/O with the provided configuration
 *
 * @param[in]   config  Pointer to the fake IO configuration
 * @param[out]  io      Pointer to the fake IO handle to be initialized
 *
 * @return
 *       - ESP_GMF_ERR_OK  Success
 *       - other           error codes  Initialization failed
 */
esp_gmf_err_t fake_io_init(fake_io_cfg_t *config, esp_gmf_io_handle_t *io);

/**
 * @brief  Casts the fake stream I/O with the provided configuration.
 *
 * @param[in]   config  Pointer to the fake IO configuration
 * @param[out]  obj     File IO handle to be casted
 *
 * @return
 *       - ESP_GMF_ERR_OK  Success
 *       - other           error codes  Casting failed
 */
esp_gmf_err_t fake_io_cast(fake_io_cfg_t *config, esp_gmf_io_handle_t obj);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
