/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include "esp_gmf_element.h"
#include "esp_gmf_err.h"
#include "esp_gmf_pool.h"

#include "esp_gmf_ch_cvt.h"
#include "esp_gmf_bit_cvt.h"
#include "esp_gmf_rate_cvt.h"
#include "esp_gmf_sonic.h"
#include "esp_gmf_alc.h"
#include "esp_gmf_eq.h"
#include "esp_gmf_fade.h"
#include "esp_gmf_mixer.h"
#include "esp_gmf_interleave.h"
#include "esp_gmf_deinterleave.h"

static const char *TAG = "GMF_SETUP_AUD_EFFECTS";

#ifdef CONFIG_GMF_AUDIO_EFFECT_INIT_ALC
static esp_gmf_err_t gmf_loader_setup_default_alc(esp_gmf_pool_handle_t pool)
{
    ESP_GMF_NULL_CHECK(TAG, pool, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
    esp_gmf_element_handle_t hd = NULL;
    esp_ae_alc_cfg_t alc_cfg = DEFAULT_ESP_GMF_ALC_CONFIG();
    ret = esp_gmf_alc_init(&alc_cfg, &hd);
    ESP_GMF_RET_ON_ERROR(TAG, ret, return ret, "Failed to init audio ALC");
    ret = esp_gmf_pool_register_element(pool, hd, NULL);
    ESP_GMF_RET_ON_ERROR(TAG, ret, {esp_gmf_element_deinit(hd); return ret;}, "Failed to register element in pool");
    return ret;
}
#endif  /* CONFIG_GMF_AUDIO_EFFECT_INIT_ALC */

#ifdef CONFIG_GMF_AUDIO_EFFECT_INIT_EQ
static esp_gmf_err_t gmf_loader_setup_default_eq(esp_gmf_pool_handle_t pool)
{
    ESP_GMF_NULL_CHECK(TAG, pool, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
    esp_gmf_element_handle_t hd = NULL;
    esp_ae_eq_cfg_t eq_cfg = DEFAULT_ESP_GMF_EQ_CONFIG();
    ret = esp_gmf_eq_init(&eq_cfg, &hd);
    ESP_GMF_RET_ON_ERROR(TAG, ret, return ret, "Failed to init audio EQ");
    ret = esp_gmf_pool_register_element(pool, hd, NULL);
    ESP_GMF_RET_ON_ERROR(TAG, ret, {esp_gmf_element_deinit(hd); return ret;}, "Failed to register element in pool");
    return ret;
}
#endif  /* CONFIG_GMF_AUDIO_EFFECT_INIT_EQ */

#ifdef CONFIG_GMF_AUDIO_EFFECT_INIT_CH_CVT
static esp_gmf_err_t gmf_loader_setup_default_ch_cvt(esp_gmf_pool_handle_t pool)
{
    ESP_GMF_NULL_CHECK(TAG, pool, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
    esp_gmf_element_handle_t hd = NULL;
    esp_ae_ch_cvt_cfg_t es_ch_cvt_cfg = DEFAULT_ESP_GMF_CH_CVT_CONFIG();
    es_ch_cvt_cfg.dest_ch = CONFIG_GMF_AUDIO_EFFECT_CH_CVT_DEST_CH;
    ret = esp_gmf_ch_cvt_init(&es_ch_cvt_cfg, &hd);
    ESP_GMF_RET_ON_ERROR(TAG, ret, return ret, "Failed to init audio ch cvt");
    ret = esp_gmf_pool_register_element(pool, hd, NULL);
    ESP_GMF_RET_ON_ERROR(TAG, ret, {esp_gmf_element_deinit(hd); return ret;}, "Failed to register element in pool");
    return ret;
}
#endif  /* CONFIG_GMF_AUDIO_EFFECT_INIT_CH_CVT */

#ifdef CONFIG_GMF_AUDIO_EFFECT_INIT_BIT_CVT
static esp_gmf_err_t gmf_loader_setup_default_bit_cvt(esp_gmf_pool_handle_t pool)
{
    ESP_GMF_NULL_CHECK(TAG, pool, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
    esp_gmf_element_handle_t hd = NULL;
    esp_ae_bit_cvt_cfg_t es_bit_cvt_cfg = DEFAULT_ESP_GMF_BIT_CVT_CONFIG();
    es_bit_cvt_cfg.dest_bits = CONFIG_GMF_AUDIO_EFFECT_BIT_CVT_DEST_BITS;
    ret = esp_gmf_bit_cvt_init(&es_bit_cvt_cfg, &hd);
    ESP_GMF_RET_ON_ERROR(TAG, ret, return ret, "Failed to init audio bit cvt");
    ret = esp_gmf_pool_register_element(pool, hd, NULL);
    ESP_GMF_RET_ON_ERROR(TAG, ret, {esp_gmf_element_deinit(hd); return ret;}, "Failed to register element in pool");
    return ret;
}
#endif  /* CONFIG_GMF_AUDIO_EFFECT_INIT_BIT_CVT */

#ifdef CONFIG_GMF_AUDIO_EFFECT_INIT_RATE_CVT
static esp_gmf_err_t gmf_loader_setup_default_rate_cvt(esp_gmf_pool_handle_t pool)
{
    ESP_GMF_NULL_CHECK(TAG, pool, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
    esp_gmf_element_handle_t hd = NULL;
    esp_ae_rate_cvt_cfg_t es_rate_cvt_cfg = DEFAULT_ESP_GMF_RATE_CVT_CONFIG();
    es_rate_cvt_cfg.dest_rate = CONFIG_GMF_AUDIO_EFFECT_RATE_CVT_DEST_RATE;
    es_rate_cvt_cfg.complexity = CONFIG_GMF_AUDIO_EFFECT_RATE_CVT_COMPLEXITY;
    es_rate_cvt_cfg.perf_type = CONFIG_GMF_AUDIO_EFFECT_RATE_CVT_PERF_TYPE;
    ret = esp_gmf_rate_cvt_init(&es_rate_cvt_cfg, &hd);
    ESP_GMF_RET_ON_ERROR(TAG, ret, return ret, "Failed to init audio rate cvt");
    ret = esp_gmf_pool_register_element(pool, hd, NULL);
    ESP_GMF_RET_ON_ERROR(TAG, ret, {esp_gmf_element_deinit(hd); return ret;}, "Failed to register element in pool");
    return ret;
}
#endif  /* CONFIG_GMF_AUDIO_EFFECT_INIT_RATE_CVT */

#ifdef CONFIG_GMF_AUDIO_EFFECT_INIT_FADE
static esp_gmf_err_t gmf_loader_setup_default_fade(esp_gmf_pool_handle_t pool)
{
    ESP_GMF_NULL_CHECK(TAG, pool, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
    esp_gmf_element_handle_t hd = NULL;
    esp_ae_fade_cfg_t fade_cfg = DEFAULT_ESP_GMF_FADE_CONFIG();
    ret = esp_gmf_fade_init(&fade_cfg, &hd);
    ESP_GMF_RET_ON_ERROR(TAG, ret, return ret, "Failed to init audio fade");
    ret = esp_gmf_pool_register_element(pool, hd, NULL);
    ESP_GMF_RET_ON_ERROR(TAG, ret, {esp_gmf_element_deinit(hd); return ret;}, "Failed to register element in pool");
    return ret;
}
#endif  /* CONFIG_GMF_AUDIO_EFFECT_INIT_FADE */

#ifdef CONFIG_GMF_AUDIO_EFFECT_INIT_SONIC
static esp_gmf_err_t gmf_loader_setup_default_sonic(esp_gmf_pool_handle_t pool)
{
    ESP_GMF_NULL_CHECK(TAG, pool, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
    esp_gmf_element_handle_t hd = NULL;
    esp_ae_sonic_cfg_t sonic_cfg = DEFAULT_ESP_GMF_SONIC_CONFIG();
    ret = esp_gmf_sonic_init(&sonic_cfg, &hd);
    ESP_GMF_RET_ON_ERROR(TAG, ret, return ret, "Failed to init audio sonic");
    ret = esp_gmf_pool_register_element(pool, hd, NULL);
    ESP_GMF_RET_ON_ERROR(TAG, ret, {esp_gmf_element_deinit(hd); return ret;}, "Failed to register element in pool");
    return ret;
}
#endif  /* CONFIG_GMF_AUDIO_EFFECT_INIT_SONIC */

#ifdef CONFIG_GMF_AUDIO_EFFECT_INIT_DEINTERLEAVE
static esp_gmf_err_t gmf_loader_setup_default_deinterleave(esp_gmf_pool_handle_t pool)
{
    ESP_GMF_NULL_CHECK(TAG, pool, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
    esp_gmf_element_handle_t hd = NULL;
    esp_gmf_deinterleave_cfg deinterleave_cfg = DEFAULT_ESP_GMF_DEINTERLEAVE_CONFIG();
    ret = esp_gmf_deinterleave_init(&deinterleave_cfg, &hd);
    ESP_GMF_RET_ON_ERROR(TAG, ret, return ret, "Failed to init audio deinterleave");
    ret = esp_gmf_pool_register_element(pool, hd, NULL);
    ESP_GMF_RET_ON_ERROR(TAG, ret, {esp_gmf_element_deinit(hd); return ret;}, "Failed to register element in pool");
    return ret;
}
#endif  /* CONFIG_GMF_AUDIO_EFFECT_INIT_DEINTERLEAVE */

#ifdef CONFIG_GMF_AUDIO_EFFECT_INIT_INTERLEAVE
static esp_gmf_err_t gmf_loader_setup_default_interleave(esp_gmf_pool_handle_t pool)
{
    ESP_GMF_NULL_CHECK(TAG, pool, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
    esp_gmf_element_handle_t hd = NULL;
    esp_gmf_interleave_cfg interleave_cfg = DEFAULT_ESP_GMF_INTERLEAVE_CONFIG();
    ret = esp_gmf_interleave_init(&interleave_cfg, &hd);
    ESP_GMF_RET_ON_ERROR(TAG, ret, return ret, "Failed to init audio interleave");
    ret = esp_gmf_pool_register_element(pool, hd, NULL);
    ESP_GMF_RET_ON_ERROR(TAG, ret, {esp_gmf_element_deinit(hd); return ret;}, "Failed to register element in pool");
    return ret;
}
#endif  /* CONFIG_GMF_AUDIO_EFFECT_INIT_INTERLEAVE */

#ifdef CONFIG_GMF_AUDIO_EFFECT_INIT_MIXER
static esp_gmf_err_t gmf_loader_setup_default_mixer(esp_gmf_pool_handle_t pool)
{
    ESP_GMF_NULL_CHECK(TAG, pool, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
    esp_gmf_element_handle_t hd = NULL;
    esp_ae_mixer_cfg_t mixer_cfg = DEFAULT_ESP_GMF_MIXER_CONFIG();
    ret = esp_gmf_mixer_init(&mixer_cfg, &hd);
    ESP_GMF_RET_ON_ERROR(TAG, ret, return ret, "Failed to init audio mixer");
    ret = esp_gmf_pool_register_element(pool, hd, NULL);
    ESP_GMF_RET_ON_ERROR(TAG, ret, {esp_gmf_element_deinit(hd); return ret;}, "Failed to register element in pool");
    return ret;
}
#endif  /* CONFIG_GMF_AUDIO_EFFECT_INIT_MIXER */

esp_gmf_err_t gmf_loader_setup_audio_effects_default(esp_gmf_pool_handle_t pool)
{
    ESP_GMF_NULL_CHECK(TAG, pool, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;

#ifdef CONFIG_GMF_AUDIO_EFFECT_INIT_ALC
    ret = gmf_loader_setup_default_alc(pool);
    ESP_GMF_RET_ON_ERROR(TAG, ret, return ret, "Failed to register ALC");
#endif  /* CONFIG_GMF_AUDIO_EFFECT_INIT_ALC */

#ifdef CONFIG_GMF_AUDIO_EFFECT_INIT_EQ
    ret = gmf_loader_setup_default_eq(pool);
    ESP_GMF_RET_ON_ERROR(TAG, ret, return ret, "Failed to register EQ");
#endif  /* CONFIG_GMF_AUDIO_EFFECT_INIT_EQ */

#ifdef CONFIG_GMF_AUDIO_EFFECT_INIT_CH_CVT
    ret = gmf_loader_setup_default_ch_cvt(pool);
    ESP_GMF_RET_ON_ERROR(TAG, ret, return ret, "Failed to register ch cvt");
#endif  /* CONFIG_GMF_AUDIO_EFFECT_INIT_CH_CVT */

#ifdef CONFIG_GMF_AUDIO_EFFECT_INIT_BIT_CVT
    ret = gmf_loader_setup_default_bit_cvt(pool);
    ESP_GMF_RET_ON_ERROR(TAG, ret, return ret, "Failed to register bit cvt");
#endif  /* CONFIG_GMF_AUDIO_EFFECT_INIT_BIT_CVT */

#ifdef CONFIG_GMF_AUDIO_EFFECT_INIT_RATE_CVT
    ret = gmf_loader_setup_default_rate_cvt(pool);
    ESP_GMF_RET_ON_ERROR(TAG, ret, return ret, "Failed to register rate cvt");
#endif  /* CONFIG_GMF_AUDIO_EFFECT_INIT_RATE_CVT */

#ifdef CONFIG_GMF_AUDIO_EFFECT_INIT_FADE
    ret = gmf_loader_setup_default_fade(pool);
    ESP_GMF_RET_ON_ERROR(TAG, ret, return ret, "Failed to register fade");
#endif  /* CONFIG_GMF_AUDIO_EFFECT_INIT_FADE */

#ifdef CONFIG_GMF_AUDIO_EFFECT_INIT_SONIC
    ret = gmf_loader_setup_default_sonic(pool);
    ESP_GMF_RET_ON_ERROR(TAG, ret, return ret, "Failed to register sonic");
#endif  /* CONFIG_GMF_AUDIO_EFFECT_INIT_SONIC */

#ifdef CONFIG_GMF_AUDIO_EFFECT_INIT_DEINTERLEAVE
    ret = gmf_loader_setup_default_deinterleave(pool);
    ESP_GMF_RET_ON_ERROR(TAG, ret, return ret, "Failed to register deinterleave");
#endif  /* CONFIG_GMF_AUDIO_EFFECT_INIT_DEINTERLEAVE */

#ifdef CONFIG_GMF_AUDIO_EFFECT_INIT_INTERLEAVE
    ret = gmf_loader_setup_default_interleave(pool);
    ESP_GMF_RET_ON_ERROR(TAG, ret, return ret, "Failed to register interleave");
#endif  /* CONFIG_GMF_AUDIO_EFFECT_INIT_INTERLEAVE */

#ifdef CONFIG_GMF_AUDIO_EFFECT_INIT_MIXER
    ret = gmf_loader_setup_default_mixer(pool);
    ESP_GMF_RET_ON_ERROR(TAG, ret, return ret, "Failed to register mixer");
#endif  /* CONFIG_GMF_AUDIO_EFFECT_INIT_MIXER */

    return ret;
}

esp_gmf_err_t gmf_loader_teardown_audio_effects_default(esp_gmf_pool_handle_t pool)
{
    ESP_GMF_NULL_CHECK(TAG, pool, return ESP_GMF_ERR_INVALID_ARG);
    return ESP_GMF_ERR_OK;
}
