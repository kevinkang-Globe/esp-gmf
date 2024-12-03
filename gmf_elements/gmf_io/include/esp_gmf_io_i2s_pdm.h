/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2025 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD.>
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

#include "driver/i2s_pdm.h"
#include "esp_gmf_io.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief  I2S PDM IO configurations, if any entry is zero then the configuration will be set to default values
 */
typedef struct {
    i2s_chan_handle_t pdm_chan; /*!<  I2S tx channel handler */
    int               dir;      /*!< IO direction, reader or writer */
    const char       *name;     /*!< Name for this instance */
} i2s_pdm_io_cfg_t;

#define ESP_GMF_IO_I2S_PDM_CFG_DEFAULT() {  \
    .pdm_chan = NULL,                       \
    .dir      = ESP_GMF_IO_DIR_NONE,        \
    .name     = NULL,                       \
}

/**
 * @brief  Initializes the I2S PDM I/O with the provided configuration
 *
 * @param[in]   config  Pointer to the file IO configuration
 * @param[out]  io      Pointer to the file IO handle to be initialized
 *
 * @return
 *       - ESP_GMF_ERR_OK           Success
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid configuration provided
 *       - ESP_GMF_ERR_MEMORY_LACK  Failed to allocate memory
 */
esp_gmf_err_t esp_gmf_io_i2s_pdm_init(i2s_pdm_io_cfg_t *config, esp_gmf_io_handle_t *io);

/**
 * @brief  Casts the I2S PDM I/O with the provided configuration.
 *
 * @param[in]   config  Pointer to the file IO configuration
 * @param[out]  obj     I2S PDM IO handle to be casted
 *
 * @return
 *       - ESP_GMF_ERR_OK           Success
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid configuration provided
 *       - ESP_GMF_ERR_MEMORY_LACK  Failed to allocate memory
 */
esp_gmf_err_t esp_gmf_io_i2s_pdm_cast(i2s_pdm_io_cfg_t *config, esp_gmf_io_handle_t obj);

#ifdef __cplusplus
}
#endif /* __cplusplus */
