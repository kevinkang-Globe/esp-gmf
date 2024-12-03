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
#include "esp_ae_sonic.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define DEFAULT_ESP_GMF_SONIC_CONFIG() {  \
    .sample_rate     = 48000,             \
    .channel         = 2,                 \
    .bits_per_sample = 16,                \
}

/**
 * @brief  Initializes the GMF sonic with the provided configuration
 *
 * @param[in]   config  Pointer to the sonic configuration
 * @param[out]  handle  Pointer to the sonic handle to be initialized
 *
 * @return
 *       - ESP_GMF_ERR_OK           Success
 *       - ESP_ERR_INVALID_ARG      Invalid configuration provided
 *       - ESP_GMF_ERR_MEMORY_LACK  Failed to allocate memory
 */
esp_gmf_err_t esp_gmf_sonic_init(esp_ae_sonic_cfg_t *config, esp_gmf_obj_handle_t *handle);

/**
 * @brief  Initializes the GMF sonic with the provided configuration
 *
 * @param[in]   config  Pointer to the sonic configuration
 * @param[out]  handle  Pointer to the sonic handle to be initialized
 *
 * @return
 *       - ESP_GMF_ERR_OK           Success
 *       - ESP_ERR_INVALID_ARG      Invalid configuration provided
 *       - ESP_GMF_ERR_MEMORY_LACK  Failed to allocate memory
 */
esp_gmf_err_t esp_gmf_sonic_cast(esp_ae_sonic_cfg_t *config, esp_gmf_obj_handle_t handle);

/**
 * @brief  Set the audio speed
 *
 * @param[in]  handle  The handle of the sonic
 * @param[in]  speed   The scaling factor of audio speed.
 *                     The range of speed is [0.5, 2.0]
 *
 * @return
 *       - ESP_GMF_ERR_OK           Operation succeeded
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid input parameter
 */
esp_gmf_err_t esp_gmf_sonic_set_speed(esp_gmf_audio_element_handle_t handle, float speed);

/**
 * @brief  Get the audio speed
 *
 * @param[in]   handle  The handle of the sonic
 * @param[out]  speed   The scaling factor of audio speed
 *
 * @return
 *       - ESP_GMF_ERR_OK           Operation succeeded
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid input parameter
 */
esp_gmf_err_t esp_gmf_sonic_get_speed(esp_gmf_audio_element_handle_t handle, float *speed);

/**
 * @brief  Set the audio pitch
 *
 * @param[in]  handle  The handle of the sonic
 * @param[in]  pitch   The scaling factor of audio pitch.
 *                     The range of pitch is [0.5, 2.0].
 *                     If the pitch value is smaller than 1.0, the sound is deep voice;
 *                     if the pitch value is equal to 1.0, the sound is no change;
 *                     if the pitch value is gather than 1.0, the sound is sharp voice;
 *
 * @return
 *       - ESP_GMF_ERR_OK           Operation succeeded
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid input parameter
 */
esp_gmf_err_t esp_gmf_sonic_set_pitch(esp_gmf_audio_element_handle_t handle, float pitch);

/**
 * @brief  Get the audio pitch
 *
 * @param[in]   handle  The handle of the sonic
 * @param[out]  pitch   The scaling factor of audio pitch
 *
 * @return
 *       - ESP_GMF_ERR_OK           Operation succeeded
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid input parameter
 */
esp_gmf_err_t esp_gmf_sonic_get_pitch(esp_gmf_audio_element_handle_t handle, float *pitch);

#ifdef __cplusplus
}
#endif /* __cplusplus */
