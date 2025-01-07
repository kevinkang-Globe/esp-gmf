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

#include "esp_gmf_err.h"
#include "esp_ae_bit_cvt.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define DEFAULT_ESP_GMF_BIT_CVT_CONFIG() {  \
    .sample_rate = 48000,                   \
    .src_bits    = 16,                      \
    .channel     = 2,                       \
    .dest_bits   = 16,                      \
}

/**
 * @brief  Initializes the GMF bit conversion with the provided configuration
 *
 * @param[in]   config  Pointer to the bit conversion configuration
 * @param[out]  handle  Pointer to the bit conversion handle to be initialized
 *
 * @return
 *       - ESP_GMF_ERR_OK           Success
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid configuration provided
 *       - ESP_GMF_ERR_MEMORY_LACK  Failed to allocate memory
 */
esp_gmf_err_t esp_gmf_bit_cvt_init(esp_ae_bit_cvt_cfg_t *config, esp_gmf_obj_handle_t *handle);

/**
 * @brief  Casts the GMF bit conversion with the provided configuration
 *
 * @param[in]   config  Pointer to the bit conversion configuration
 * @param[out]  handle  Bit conversion handle to be casted
 *
 * @return
 *       - ESP_GMF_ERR_OK           Success
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid configuration provided
 *       - ESP_GMF_ERR_MEMORY_LACK  Failed to allocate memory
 */
esp_gmf_err_t esp_gmf_bit_cvt_cast(esp_ae_bit_cvt_cfg_t *config, esp_gmf_obj_handle_t handle);

/**
 * @brief  Set destination bits for the bit conversion handle
 *         Note: Only allow set on state `ESP_GMF_EVENT_STATE_NONE` and `ESP_GMF_EVENT_STATE_INITIALIZED`
 *
 * @param[in]  handle     The channel bit handle
 * @param[in]  dest_bits  The destination bits
 *
 * @return
 *       - ESP_GMF_ERR_OK           Operation succeeded
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid input parameter
 *       - ESP_GMF_ERR_FAIL         Failed to set configuration
 */
esp_gmf_err_t esp_gmf_bit_cvt_set_dest_bits(esp_gmf_audio_element_handle_t handle, uint8_t dest_bits);

#ifdef __cplusplus
}
#endif /* __cplusplus */
