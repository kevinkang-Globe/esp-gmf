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

#include "esp_codec_dev.h"
#include "esp_gmf_io.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief  Audio Codec Device IO configurations, if any entry is zero then the configuration will be set to default values
 */
typedef struct {
    esp_codec_dev_handle_t dev;  /*!< Audio Codec Device handler */
    esp_gmf_io_dir_t       dir;  /*!< IO direction, reader or writer */
    const char            *name; /*!< Name for this instance */
} codec_dev_io_cfg_t;

#define ESP_GMF_IO_CODEC_DEV_CFG_DEFAULT() {  \
    .dev  = NULL,                             \
    .dir  = ESP_GMF_IO_DIR_NONE,              \
    .name = NULL,                             \
}

/**
 * @brief  Initializes the Audio Codec Device I/O with the provided configuration
 *
 * @param[in]   config  Audio codec device io configuration
 * @param[out]  io      Audio codec device io handle
 *
 * @return
 *       - ESP_GMF_ERR_OK           Success
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid configuration provided
 *       - ESP_GMF_ERR_MEMORY_LACK  Failed to allocate memory
 */
esp_gmf_err_t esp_gmf_io_codec_dev_init(codec_dev_io_cfg_t *config, esp_gmf_io_handle_t *io);

/**
 * @brief  Casts the Audio Codec Device I/O with the provided configuration.
 *
 * @param[in]   config  Audio codec device io configuration
 * @param[out]  obj     Audio codec device io handle to be casted
 *
 * @return
 *       - ESP_GMF_ERR_OK           Success
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid configuration provided
 *       - ESP_GMF_ERR_MEMORY_LACK  Failed to allocate memory
 */
esp_gmf_err_t esp_gmf_io_codec_dev_cast(codec_dev_io_cfg_t *config, esp_gmf_io_handle_t obj);

#ifdef __cplusplus
}
#endif /* __cplusplus */
