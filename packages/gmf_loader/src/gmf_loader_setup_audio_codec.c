/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include "esp_audio_dec_default.h"
#include "esp_audio_enc_default.h"
#include "esp_audio_simple_dec_default.h"
#include "esp_gmf_audio_dec.h"
#include "esp_gmf_audio_enc.h"
#include "esp_gmf_element.h"
#include "esp_gmf_err.h"
#include "esp_gmf_pool.h"

static const char *TAG = "GMF_SETUP_AUD_CODEC";
static uint32_t setup_cnt = 0;

#if defined(CONFIG_GMF_AUDIO_CODEC_INIT_ENCODER)
static inline esp_gmf_err_t __default_enc_config(esp_audio_enc_config_t *enc_cfg)
{
#ifdef CONFIG_GMF_AUDIO_CODEC_ENCODER_TYPE_AAC
    enc_cfg->type = ESP_AUDIO_TYPE_AAC;
    esp_aac_enc_config_t aac_enc_cfg = ESP_AAC_ENC_CONFIG_DEFAULT();
    aac_enc_cfg.bitrate = CONFIG_GMF_AUDIO_CODEC_ENC_AAC_BITRATE;
#ifdef CONFIG_GMF_AUDIO_CODEC_ENC_AAC_ADTS_USED
    aac_enc_cfg.adts_used = true;
#else
    aac_enc_cfg.adts_used = false;
#endif  /* CONFIG_GMF_AUDIO_CODEC_ENC_AAC_ADTS_USED */
    enc_cfg->cfg = &aac_enc_cfg;
    enc_cfg->cfg_sz = sizeof(esp_aac_enc_config_t);
#elif defined(CONFIG_GMF_AUDIO_CODEC_ENCODER_TYPE_OPUS)
    enc_cfg->type = ESP_AUDIO_TYPE_OPUS;
    esp_opus_enc_config_t opus_enc_cfg = ESP_OPUS_ENC_CONFIG_DEFAULT();
    opus_enc_cfg.bitrate = CONFIG_GMF_AUDIO_CODEC_ENC_OPUS_BITRATE;
    opus_enc_cfg.frame_duration = CONFIG_GMF_AUDIO_CODEC_ENC_OPUS_FRAME_DURATION;
    opus_enc_cfg.application_mode = CONFIG_GMF_AUDIO_CODEC_ENC_OPUS_APPLICATION_MODE;
    opus_enc_cfg.complexity = CONFIG_GMF_AUDIO_CODEC_ENC_OPUS_COMPLEXITY;
#ifdef CONFIG_GMF_AUDIO_CODEC_ENC_OPUS_ENABLE_FEC
    opus_enc_cfg.enable_fec = true;
#else
    opus_enc_cfg.enable_fec = false;
#endif
#ifdef CONFIG_GMF_AUDIO_CODEC_ENC_OPUS_ENABLE_DTX
    opus_enc_cfg.enable_dtx = true;
#else
    opus_enc_cfg.enable_dtx = false;
#endif
#ifdef CONFIG_GMF_AUDIO_CODEC_ENC_OPUS_ENABLE_VBR
    opus_enc_cfg.enable_vbr = true;
#else
    opus_enc_cfg.enable_vbr = false;
#endif
    enc_cfg->cfg = &opus_enc_cfg;
    enc_cfg->cfg_sz = sizeof(esp_opus_enc_config_t);
    return ESP_GMF_ERR_OK;
#elif defined(CONFIG_GMF_AUDIO_CODEC_ENCODER_TYPE_ADPCM)
    enc_cfg->type = ESP_AUDIO_TYPE_ADPCM;
    esp_adpcm_enc_config_t adpcm_enc_cfg = ESP_ADPCM_ENC_CONFIG_DEFAULT();
    enc_cfg = &adpcm_enc_cfg;
    enc_cfg->cfg_sz = sizeof(esp_adpcm_enc_config_t);
    return ESP_GMF_ERR_OK;
#elif defined(CONFIG_GMF_AUDIO_CODEC_ENCODER_TYPE_ALAC)
    enc_cfg->type = ESP_AUDIO_TYPE_ALAC;
    esp_alac_enc_config_t alac_enc_cfg = ESP_ALAC_ENC_CONFIG_DEFAULT();
    enc_cfg->cfg = &alac_enc_cfg;
    enc_cfg->cfg_sz = sizeof(esp_alac_enc_config_t);
    return ESP_GMF_ERR_OK;
#elif defined(CONFIG_GMF_AUDIO_CODEC_ENCODER_TYPE_PCM)
    enc_cfg->type = ESP_AUDIO_TYPE_PCM;
    esp_pcm_enc_config_t pcm_enc_cfg = ESP_PCM_ENC_CONFIG_DEFAULT();
    enc_cfg->cfg = &pcm_enc_cfg;
    enc_cfg->cfg_sz = sizeof(esp_pcm_enc_config_t);
    return ESP_GMF_ERR_OK;
#elif defined(CONFIG_GMF_AUDIO_CODEC_ENCODER_TYPE_G711A)
    enc_cfg->type = ESP_AUDIO_TYPE_G711A;
    esp_g711_enc_config_t g711_enc_cfg = ESP_G711_ENC_CONFIG_DEFAULT();
    enc_cfg->cfg = &g711_enc_cfg;
    enc_cfg->cfg_sz = sizeof(esp_g711_enc_config_t);
#elif defined(CONFIG_GMF_AUDIO_CODEC_ENCODER_TYPE_G711U)
    enc_cfg->type = ESP_AUDIO_TYPE_G711U;
    esp_g711_enc_config_t g711_enc_cfg = ESP_G711_ENC_CONFIG_DEFAULT();
    enc_cfg->cfg = &g711_enc_cfg;
    enc_cfg->cfg_sz = sizeof(esp_g711_enc_config_t);
#elif defined(CONFIG_GMF_AUDIO_CODEC_ENCODER_TYPE_AMRNB)
    enc_cfg->type = ESP_AUDIO_TYPE_AMRNB;
    esp_amrnb_enc_config_t amr_enc_cfg = ESP_AMRNB_ENC_CONFIG_DEFAULT();
    amr_enc_cfg.bitrate_mode = CONFIG_GMF_AUDIO_CODEC_ENC_AMRNB_BITRATE_MODE;
#ifdef CONFIG_GMF_AUDIO_CODEC_ENC_AMRNB_NO_FILE_HEADER
    amr_enc_cfg.no_file_header = true;
#else
    amr_enc_cfg.no_file_header = false;
#endif  /* CONFIG_GMF_AUDIO_CODEC_ENC_AMRNB_NO_FILE_HEADER */
    enc_cfg->cfg = &amr_enc_cfg;
    enc_cfg->cfg_sz = sizeof(esp_amrnb_enc_config_t);
#elif defined(CONFIG_GMF_AUDIO_CODEC_ENCODER_TYPE_AMRWB)
    enc_cfg->type = ESP_AUDIO_TYPE_AMRWB;
    esp_amrwb_enc_config_t amr_enc_cfg = ESP_AMRWB_ENC_CONFIG_DEFAULT();
    amr_enc_cfg.bitrate_mode = CONFIG_GMF_AUDIO_CODEC_ENC_AMRWB_BITRATE_MODE;
#ifdef CONFIG_GMF_AUDIO_CODEC_ENC_AMRWB_NO_FILE_HEADER
    amr_enc_cfg.no_file_header = true;
#else
    amr_enc_cfg.no_file_header = false;
#endif  /* CONFIG_GMF_AUDIO_CODEC_ENC_AMRWB_NO_FILE_HEADER */
    enc_cfg->cfg = &amr_enc_cfg;
    enc_cfg->cfg_sz = sizeof(esp_amrwb_enc_config_t);
#else  /*CONFIG_GMF_AUDIO_CODEC_ENCODER_TYPE_AAC*/
    enc_cfg->type = ESP_AUDIO_TYPE_UNSUPPORT;
    return ESP_GMF_ERR_NOT_SUPPORT;
#endif  /*CONFIG_GMF_AUDIO_CODEC_ENCODER_TYPE_AAC*/
    return ESP_GMF_ERR_OK;
}

static esp_gmf_err_t gmf_loader_setup_default_enc(esp_gmf_pool_handle_t pool)
{
    ESP_GMF_NULL_CHECK(TAG, pool, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
    esp_gmf_element_handle_t hd = NULL;

    if (setup_cnt == 0) {
        ret = esp_audio_enc_register_default();
        ESP_GMF_RET_ON_ERROR(TAG, ret, return ret, "Failed to register audio encoders");
    }
    esp_audio_enc_config_t enc_cfg = DEFAULT_ESP_GMF_AUDIO_ENC_CONFIG();
    ret = __default_enc_config(&enc_cfg);
    ESP_GMF_RET_ON_ERROR(TAG, ret, return ret, "Failed to set default encoder config");
    ret = esp_gmf_audio_enc_init(&enc_cfg, &hd);
    ESP_GMF_RET_ON_ERROR(TAG, ret, return ret, "Failed to init audio enc");
    ret = esp_gmf_pool_register_element(pool, hd, NULL);
    ESP_GMF_RET_ON_ERROR(TAG, ret, {esp_gmf_element_deinit(hd); return ret;}, "Failed to register element in pool");
    return ret;
}
#endif  /* CONFIG_GMF_AUDIO_CODEC_INIT_ENCODER */

#if defined(CONFIG_GMF_AUDIO_CODEC_INIT_DECODER)
static inline esp_gmf_err_t __default_dec_config(esp_audio_simple_dec_cfg_t *dec_cfg)
{
#ifdef CONFIG_GMF_AUDIO_CODEC_DECODER_TYPE_AAC
    dec_cfg->dec_type = ESP_AUDIO_SIMPLE_DEC_TYPE_AAC;
    esp_aac_dec_cfg_t aac_dec_cfg = {0};
#if defined(CONFIG_GMF_AUDIO_CODEC_DEC_AAC_NO_ADTS_HEADER)
    aac_dec_cfg.no_adts_header = true;
#else
    aac_dec_cfg.no_adts_header = false;
#endif  /* defined(CONFIG_GMF_AUDIO_CODEC_DEC_AAC_NO_ADTS_HEADER) */
#if defined(CONFIG_GMF_AUDIO_CODEC_DEC_AAC_PLUS_ENABLE)
    aac_dec_cfg.aac_plus_enable = true;
#else
    aac_dec_cfg.aac_plus_enable = false;
#endif  /* defined(CONFIG_GMF_AUDIO_CODEC_DEC_AAC_PLUS_ENABLE) */
    dec_cfg->dec_cfg = &aac_dec_cfg;
    dec_cfg->cfg_size = sizeof(esp_aac_dec_cfg_t);
#elif defined(CONFIG_GMF_AUDIO_CODEC_DECODER_TYPE_MP3)
    dec_cfg->dec_type = ESP_AUDIO_SIMPLE_DEC_TYPE_MP3;
#elif defined(CONFIG_GMF_AUDIO_CODEC_DECODER_TYPE_AMRNB)
    dec_cfg->dec_type = ESP_AUDIO_SIMPLE_DEC_TYPE_AMRNB;
#elif defined(CONFIG_GMF_AUDIO_CODEC_DECODER_TYPE_AMRWB)
    dec_cfg->dec_type = ESP_AUDIO_SIMPLE_DEC_TYPE_AMRWB;
#elif defined(CONFIG_GMF_AUDIO_CODEC_DECODER_TYPE_FLAC)
    dec_cfg->dec_type = ESP_AUDIO_SIMPLE_DEC_TYPE_FLAC;
#elif defined(CONFIG_GMF_AUDIO_CODEC_DECODER_TYPE_WAV)
    dec_cfg->dec_type = ESP_AUDIO_SIMPLE_DEC_TYPE_WAV;
#elif defined(CONFIG_GMF_AUDIO_CODEC_DECODER_TYPE_M4A)
    dec_cfg->dec_type = ESP_AUDIO_SIMPLE_DEC_TYPE_M4A;
#elif defined(CONFIG_GMF_AUDIO_CODEC_DECODER_TYPE_TS)
    dec_cfg->dec_type = ESP_AUDIO_SIMPLE_DEC_TYPE_TS;
#elif defined(CONFIG_GMF_AUDIO_CODEC_DECODER_TYPE_RAW_OPUS)
    dec_cfg->dec_type = ESP_AUDIO_SIMPLE_DEC_TYPE_RAW_OPUS;
    esp_opus_dec_cfg_t opus_dec_cfg = ESP_OPUS_DEC_CONFIG_DEFAULT();
    opus_dec_cfg.sample_rate = CONFIG_GMF_AUDIO_CODEC_DEC_OPUS_SAMPLE_RATE;
    opus_dec_cfg.channel = CONFIG_GMF_AUDIO_CODEC_DEC_OPUS_CHANNEL;
    opus_dec_cfg.frame_duration = CONFIG_GMF_AUDIO_CODEC_DEC_OPUS_FRAME_DURATION;
#if defined(CONFIG_GMF_AUDIO_CODEC_DEC_OPUS_SELF_DELIMITED)
    opus_dec_cfg.self_delimited = true;
#endif  /* defined(CONFIG_GMF_AUDIO_CODEC_DEC_OPUS_SELF_DELIMITED) */
    dec_cfg->dec_cfg = &opus_dec_cfg;
    dec_cfg->cfg_size = sizeof(esp_opus_dec_cfg_t);
#elif defined(CONFIG_GMF_AUDIO_CODEC_DECODER_TYPE_G711A)
    dec_cfg->dec_type = ESP_AUDIO_SIMPLE_DEC_TYPE_G711A;
    esp_g711_dec_cfg_t g711_dec_cfg = {0};
    g711_dec_cfg.channel = CONFIG_GMF_AUDIO_CODEC_DEC_G711_CHANNEL;
    dec_cfg->dec_cfg = &g711_dec_cfg;
    dec_cfg->cfg_size = sizeof(esp_g711_dec_cfg_t);
#elif defined(CONFIG_GMF_AUDIO_CODEC_DECODER_TYPE_G711U)
    dec_cfg->dec_type = ESP_AUDIO_SIMPLE_DEC_TYPE_G711U;
    esp_g711_dec_cfg_t g711_dec_cfg = {0};
    g711_dec_cfg.channel = CONFIG_GMF_AUDIO_CODEC_DEC_G711_CHANNEL;
    dec_cfg->dec_cfg = &g711_dec_cfg;
    dec_cfg->cfg_size = sizeof(esp_g711_dec_cfg_t);
#elif defined(CONFIG_GMF_AUDIO_CODEC_DECODER_TYPE_PCM)
    dec_cfg->dec_type = ESP_AUDIO_SIMPLE_DEC_TYPE_PCM;
#elif defined(CONFIG_GMF_AUDIO_CODEC_DECODER_TYPE_ADPCM)
    dec_cfg->dec_type = ESP_AUDIO_SIMPLE_DEC_TYPE_ADPCM;
    esp_adpcm_dec_cfg_t adpcm_dec_cfg = {0};
    adpcm_dec_cfg.sample_rate = CONFIG_GMF_AUDIO_CODEC_DEC_ADPCM_SAMPLE_RATE;
    adpcm_dec_cfg.channel = CONFIG_GMF_AUDIO_CODEC_DEC_ADPCM_CHANNEL;
    adpcm_dec_cfg.bits_per_sample = 4; // IMA-ADPCM only supports 4-bit
    dec_cfg->dec_cfg = &adpcm_dec_cfg;
    dec_cfg->cfg_size = sizeof(esp_adpcm_dec_cfg_t);
#else
    dec_cfg->dec_type = ESP_AUDIO_SIMPLE_DEC_TYPE_NONE;
    return ESP_GMF_ERR_NOT_SUPPORT;
#endif  /*CONFIG_GMF_AUDIO_CODEC_DECODER_TYPE_AAC*/
    return ESP_GMF_ERR_OK;
}

static esp_gmf_err_t gmf_loader_setup_default_dec(esp_gmf_pool_handle_t pool)
{
    ESP_GMF_NULL_CHECK(TAG, pool, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
    esp_gmf_element_handle_t hd = NULL;

    if (setup_cnt == 0) {
        ret = esp_audio_dec_register_default();
        ESP_GMF_RET_ON_ERROR(TAG, ret, return ret, "Failed to register audio decoders");
        ret = esp_audio_simple_dec_register_default();
        ESP_GMF_RET_ON_ERROR(TAG, ret, return ret, "Failed to register audio simple decoders");
    }
    esp_audio_simple_dec_cfg_t dec_cfg = DEFAULT_ESP_GMF_AUDIO_DEC_CONFIG();
    ret = __default_dec_config(&dec_cfg);
    ESP_GMF_RET_ON_ERROR(TAG, ret, return ret, "Failed to set default decoder config");
    ret = esp_gmf_audio_dec_init(&dec_cfg, &hd);
    ESP_GMF_RET_ON_ERROR(TAG, ret, return ret, "Failed to init audio dec");
    ret = esp_gmf_pool_register_element(pool, hd, NULL);
    ESP_GMF_RET_ON_ERROR(TAG, ret, {esp_gmf_element_deinit(hd); return ret;}, "Failed to register element in pool");
    return ret;
}
#endif  /* CONFIG_GMF_AUDIO_CODEC_INIT_DECODER */

esp_gmf_err_t gmf_loader_setup_audio_codec_default(esp_gmf_pool_handle_t pool)
{
    ESP_GMF_NULL_CHECK(TAG, pool, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;

#ifdef CONFIG_GMF_AUDIO_CODEC_INIT_ENCODER
    ret = gmf_loader_setup_default_enc(pool);
    ESP_GMF_RET_ON_ERROR(TAG, ret, return ret, "Failed to register codec encoder");
#endif  /* CONFIG_GMF_AUDIO_CODEC_INIT_ENCODER */

#ifdef CONFIG_GMF_AUDIO_CODEC_INIT_DECODER
    ret = gmf_loader_setup_default_dec(pool);
    ESP_GMF_RET_ON_ERROR(TAG, ret, return ret, "Failed to register codec decoder");
#endif  /* CONFIG_GMF_AUDIO_CODEC_INIT_DECODER */
    setup_cnt++;
    return ret;
}

esp_gmf_err_t gmf_loader_teardown_audio_codec_default(esp_gmf_pool_handle_t pool)
{
    ESP_GMF_NULL_CHECK(TAG, pool, return ESP_GMF_ERR_INVALID_ARG);
    if (setup_cnt == 0) {
        ESP_LOGW(TAG, "Default audio codec is not initialized");
        return ESP_GMF_ERR_OK;
    }
    if ((--setup_cnt) == 0) {
#ifdef CONFIG_GMF_AUDIO_CODEC_INIT_ENCODER
        ESP_LOGW(TAG, "Unregistering default encoder");
        esp_audio_enc_unregister_default();
#endif  /* CONFIG_GMF_AUDIO_CODEC_INIT_ENCODER */

#ifdef CONFIG_GMF_AUDIO_CODEC_INIT_DECODER
        ESP_LOGW(TAG, "Unregistering default decoder");
        esp_audio_dec_unregister_default();
        esp_audio_simple_dec_unregister_default();
#endif  /* CONFIG_GMF_AUDIO_CODEC_INIT_DECODER */
        setup_cnt = 0;
    } else {
        ESP_LOGW(TAG, "Default audio codec is still in use");
    }
    return ESP_GMF_ERR_OK;
}
