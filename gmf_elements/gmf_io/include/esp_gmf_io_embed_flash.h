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

#include "esp_gmf_io.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief  Espressif embed flash configurations, if any entry is zero then the configuration will be set to default values
 */
typedef struct {
    int         max_files; /*!< IO direction, reader or writer */
    const char *name;      /*!< Name for this instance */
} embed_flash_io_cfg_t;

#define EMBED_FLASH_CFG_DEFAULT() {  \
    .max_files = 200,                \
    .name      = NULL,               \
}

/**
 * @brief  Embed information in flash
 */
typedef struct embed_item_info {
    const uint8_t *address; /*!< The corresponding address in flash */
    int            size;    /*!< Size of corresponding data */
} embed_item_info_t;

/**
 * @brief  Initializes the embed flash stream I/O with the provided configuration
 *
 * @param[in]   config  Pointer to the embed flash IO configuration
 * @param[out]  io      Pointer to the embed flash IO handle to be initialized
 *
 * @return
 *       - ESP_GMF_ERR_OK           Success
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid configuration provided
 *       - ESP_GMF_ERR_MEMORY_LACK  Failed to allocate memory
 */
esp_gmf_err_t esp_gmf_io_embed_flash_init(embed_flash_io_cfg_t *config, esp_gmf_io_handle_t *io);

/**
 * @brief  Casts the embed flash stream I/O with the provided configuration.
 *
 * @param[in]   config  Pointer to the embed flash IO configuration
 * @param[out]  io      Embed flash IO handle to be casted
 *
 * @return
 *       - ESP_GMF_ERR_OK           Success
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid configuration provided
 *       - ESP_GMF_ERR_MEMORY_LACK  Failed to allocate memory
 */
esp_gmf_err_t esp_gmf_io_embed_flash_cast(embed_flash_io_cfg_t *config, esp_gmf_io_handle_t io);

/**
 * @brief  Set the embed flash context
 *         This function mainly provides information about embed flash data
 *
 * @param[in]  io       The embed flash element handle
 * @param[in]  context  The embed flash context
 * @param[in]  max_num  The number of embed flash context
 *
 * @return
 *       - ESP_GMF_ERR_OK
 *       - ESP_GMF_ERR_FAIL
 */
esp_gmf_err_t esp_gmf_io_embed_flash_set_context(esp_gmf_io_handle_t io, const embed_item_info_t *context, int max_num);

#ifdef __cplusplus
}
#endif /* __cplusplus */
