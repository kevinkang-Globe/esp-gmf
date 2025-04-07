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
#include "esp_ae_eq.h"
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief  Default EQ parameters
 *         If `para` is NULL in the `esp_ae_eq_cfg_t` , the EQ will use `esp_gmf_default_eq_paras` as default
 *         And the `filter_num` will be set to sizeof(esp_gmf_default_eq_paras) / sizeof(esp_ae_eq_filter_para_t)
 */
#define DEFAULT_ESP_GMF_EQ_CONFIG() {  \
    .sample_rate     = 48000,          \
    .bits_per_sample = 16,             \
    .channel         = 2,              \
    .filter_num      = 0,              \
    .para            = NULL,           \
}

/**
 * @brief  Initializes the GMF EQ with the provided configuration
 *
 * @param[in]   config  Pointer to the EQ configuration
 * @param[out]  handle  Pointer to the EQ handle to be initialized
 *
 * @return
 *       - ESP_GMF_ERR_OK           Success
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid configuration provided
 *       - ESP_GMF_ERR_MEMORY_LACK  Failed to allocate memory
 */
esp_gmf_err_t esp_gmf_eq_init(esp_ae_eq_cfg_t *config, esp_gmf_obj_handle_t *handle);

/**
 * @brief  Initializes the GMF EQ with the provided configuration
 *
 * @param[in]   config  Pointer to the EQ configuration
 * @param[out]  handle  Pointer to the EQ handle to be initialized
 *
 * @return
 *       - ESP_GMF_ERR_OK           Success
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid configuration provided
 *       - ESP_GMF_ERR_MEMORY_LACK  Failed to allocate memory
 */
esp_gmf_err_t esp_gmf_eq_cast(esp_ae_eq_cfg_t *config, esp_gmf_obj_handle_t handle);

/**
 * @brief  Set the filter parameters for a specific filter identified by 'idx'
 *
 * @param[in]  handle  The EQ handle
 * @param[in]  idx     The index of a specific filter for which the parameters are to be set.
 *                     eg: 0 refers to the first filter
 * @param[in]  para    The filter setup parameter
 *
 * @return
 *       - ESP_GMF_ERR_OK           Operation succeeded
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid input parameter
 */
esp_gmf_err_t esp_gmf_eq_set_para(esp_gmf_audio_element_handle_t handle, uint8_t idx, esp_ae_eq_filter_para_t *para);

/**
 * @brief  Get the filter parameters for a specific filter identified by 'idx'
 *
 * @param[in]   handle  The EQ handle
 * @param[in]   idx     The index of a specific filter for which the parameters are to be retrieved.
 *                      eg: 0 refers to first filter
 * @param[out]  para    The filter setup parameter
 *
 * @return
 *       - ESP_GMF_ERR_OK           Operation succeeded
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid input parameter
 */
esp_gmf_err_t esp_gmf_eq_get_para(esp_gmf_audio_element_handle_t handle, uint8_t idx, esp_ae_eq_filter_para_t *para);

/**
 * @brief  Choose to enable or disable filter processing for a specific filter identified by 'idx' in the equalizer
 *
 * @param[in]  handle     The EQ handle
 * @param[in]  idx        The index of a specific filter to be enabled
 * @param[in]  is_enable  The flag of whether to enable band filter processing
 *
 * @return
 *       - ESP_GMF_ERR_OK           Operation succeeded
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid input parameter
 */
esp_gmf_err_t esp_gmf_eq_enable_filter(esp_gmf_audio_element_handle_t handle, uint8_t idx, bool is_enable);

#ifdef __cplusplus
}
#endif /* __cplusplus */
