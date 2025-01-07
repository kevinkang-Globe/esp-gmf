/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2025 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
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

#include "esp_gmf_audio_enc.h"
#include "esp_audio_simple_dec.h"
#include "esp_gmf_info.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief  Reconfigurate information of audio encoder by type and input sound information
 *
 * @param[in]   type     Type of audio encoder
 * @param[in]   info     Information of audio encoder
 * @param[out]  enc_cfg  Configuration of audio encoder
 *
 * @return
 *       - ESP_GMF_ERR_OK           Success
 *       - ESP_GMF_ERR_NOT_SUPPORT  Not supported encoder type
 *       - ESP_GMF_ERR_MEMORY_LACK  Failed to allocate memory
 */
esp_gmf_err_t esp_gmf_audio_helper_reconfig_enc_by_type(esp_audio_type_t type, esp_gmf_info_sound_t *info,
                                                        esp_audio_enc_config_t *enc_cfg);

/**
 * @brief  Get type of audio encoder by uri
 *
 * @param[in]   uri   URI of audio encoder
 * @param[out]  type  Type of audio encoder
 *
 * @return
 *       - ESP_GMF_ERR_OK           Success
 *       - ESP_GMF_ERR_NOT_SUPPORT  Not supported encoder type
 */
esp_gmf_err_t esp_gmf_audio_helper_get_audio_type_by_uri(const char *uri, esp_audio_type_t *type);

/**
 * @brief  Reconfigurate type of audio decoder by URI
 *
 * @param[in]   uri      URI of audio decoder
 * @param[out]  dec_cfg  Configuration of audio encoder
 *
 * @return
 *       - ESP_GMF_ERR_OK           Success
 *       - ESP_GMF_ERR_NOT_SUPPORT  Not supported encoder type
 *       - ESP_GMF_ERR_MEMORY_LACK  Failed to allocate memory
 */
esp_gmf_err_t esp_gmf_audio_helper_reconfig_dec_by_uri(const char *uri, esp_audio_simple_dec_cfg_t *dec_cfg);

#ifdef __cplusplus
}
#endif /* __cplusplus */
