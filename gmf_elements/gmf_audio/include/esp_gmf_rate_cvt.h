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
#include "esp_ae_rate_cvt.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define DEFAULT_ESP_GMF_RATE_CVT_CONFIG() {              \
    .src_rate        = 44100,                            \
    .bits_per_sample = 16,                               \
    .channel         = 2,                                \
    .dest_rate       = 48000,                            \
    .complexity      = 2,                                \
    .perf_type       = ESP_AE_RATE_CVT_PERF_TYPE_SPEED,  \
}

/**
 * @brief  Initializes the GMF rate conversion with the provided configuration
 *
 * @param[in]   config  Pointer to the rate conversion configuration
 * @param[out]  handle  Pointer to the rate conversion handle to be initialized
 *
 * @return
 *       - ESP_GMF_ERR_OK           Success
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid configuration provided
 *       - ESP_GMF_ERR_MEMORY_LACK  Failed to allocate memory
 */
esp_gmf_err_t esp_gmf_rate_cvt_init(esp_ae_rate_cvt_cfg_t *config, esp_gmf_obj_handle_t *handle);

/**
 * @brief  Initializes the GMF rate conversion with the provided configuration
 *
 * @param[in]   config  Pointer to the rate conversion configuration
 * @param[out]  handle  Pointer to the rate conversion handle to be initialized
 *
 * @return
 *       - ESP_GMF_ERR_OK           Success
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid configuration provided
 *       - ESP_GMF_ERR_MEMORY_LACK  Failed to allocate memory
 */
esp_gmf_err_t esp_gmf_rate_cvt_cast(esp_ae_rate_cvt_cfg_t *config, esp_gmf_obj_handle_t handle);

/**
 * @brief  Set dest rate in the rate conversion handle
 *         Note: If the state of rate conversion is not in 'ESP_GMF_EVENT_STATE_NONE' or 'ESP_GMF_EVENT_STATE_INITIALIZED',
 *         the setting will return fail.
 *
 * @param[in]  handle   The rate conversion handle
 * @param[in]  dest_ch  The dest rate
 *
 * @return
 *       - ESP_GMF_ERR_OK           Operation succeeded
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid input parameter
 *       - ESP_GMF_ERR_FAIL         Failed to set configuration
 */
esp_gmf_err_t esp_gmf_rate_cvt_set_dest_rate(esp_gmf_audio_element_handle_t handle, uint32_t dest_rate);

#ifdef __cplusplus
}
#endif /* __cplusplus */
