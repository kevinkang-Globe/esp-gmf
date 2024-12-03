/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2025 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
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
#include "unity.h"
#include <string.h>
#include "esp_log.h"
#include "esp_err.h"
#include "driver/sdmmc_host.h"
#include "esp_gmf_element.h"
#include "esp_gmf_pipeline.h"
#include "esp_gmf_pool.h"
#include "esp_gmf_oal_mem.h"
#include "esp_gmf_oal_thread.h"
#include "esp_gmf_io_http.h"
#include "esp_gmf_rate_cvt.h"
#include "esp_gmf_audio_enc.h"
#include "esp_gmf_setup_pool.h"
#include "esp_gmf_setup_peripheral.h"
#include "esp_gmf_audio_helper.h"
#ifdef MEDIA_LIB_MEM_TEST
#include "media_lib_adapter.h"
#include "media_lib_mem_trace.h"
#endif /* MEDIA_LIB_MEM_TEST */

static const char *TAG = "AUDIO_REC_ELEMENT_TEST";

static const char *test_enc_format[] = {
    "aac",
    "amrnb",
    "amrwb",
    "pcm",
    "adpcm",
    "g711",
    "opus",
    "alac",
};

static esp_err_t _pipeline_event(esp_gmf_event_pkt_t *event, void *ctx)
{
    // The warning messages are used to make the content more noticeable.
    ESP_LOGW(TAG, "CB: RECV Pipeline EVT: el:%s-%p, type:%x, sub:%s, payload:%p, size:%d,%p",
             "OBJ_GET_TAG(event->from)", event->from, event->type, esp_gmf_event_get_state_str(event->sub),
             event->payload, event->payload_size, ctx);
    return 0;
}

TEST_CASE("Recorder, One Pipe, [IIS->ENC->FILE]", "ESP_GMF_POOL")
{
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("ESP_GMF_PIPELINE", ESP_LOG_DEBUG);
    esp_log_level_set("ESP_GMF_POOL", ESP_LOG_DEBUG);
    // esp_log_level_set("ESP_GMF_PORT", ESP_LOG_DEBUG);
    ESP_GMF_MEM_SHOW(TAG);

    void *sdcard = NULL;
    esp_gmf_setup_periph_sdmmc(&sdcard);
    esp_gmf_setup_periph_i2c(0);
    esp_gmf_setup_periph_aud_info rec_info = {
        .sample_rate = 16000,
        .channel = 1,
        .bits_per_sample = 16,
        .port_num = 0,
    };
    void *record_dev = NULL;
    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_setup_periph_codec(NULL, &rec_info, NULL, &record_dev));
#ifdef MEDIA_LIB_MEM_TEST
    media_lib_add_default_adapter();
#endif /* MEDIA_LIB_MEM_TEST */
    esp_gmf_pool_handle_t pool = NULL;
    esp_gmf_pool_init(&pool);
    TEST_ASSERT_NOT_NULL(pool);

    pool_register_audio_codecs(pool);
    pool_register_audio_effects(pool);
    pool_register_io(pool);
    pool_register_codec_dev_io(pool, NULL, record_dev);

    ESP_GMF_POOL_SHOW_ITEMS(pool);
    // Create the new elements
    esp_gmf_pipeline_handle_t pipe = NULL;
    const char *uri = "/sdcard/esp_gmf_rec1.aac";

    // const char *name[] = {"encoder"};
    const char *name[] = {"rate_cvt", "ch_cvt", "encoder"};
    esp_gmf_pool_new_pipeline(pool, "codec_dev_rx", name, sizeof(name) / sizeof(char *), "file", &pipe);
    TEST_ASSERT_NOT_NULL(pipe);

    esp_gmf_task_cfg_t cfg = DEFAULT_ESP_GMF_TASK_CONFIG();
    cfg.ctx = NULL;
    cfg.cb = NULL;
    esp_gmf_task_handle_t work_task = NULL;
    esp_gmf_task_init(&cfg, &work_task);
    TEST_ASSERT_NOT_NULL(work_task);

    esp_gmf_pipeline_bind_task(pipe, work_task);
    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_pipeline_loading_jobs(pipe));
    esp_gmf_pipeline_set_event(pipe, _pipeline_event, NULL);

    esp_gmf_pipeline_set_out_uri(pipe, uri);
    esp_gmf_element_handle_t enc_handle = NULL;
    esp_gmf_pipeline_get_el_by_name(pipe, "encoder", &enc_handle);
    esp_audio_type_t audio_type = 0;
    esp_gmf_audio_helper_get_audio_type_by_uri(uri, &audio_type);
    esp_gmf_info_sound_t info = {
        .sample_rates = 16000,
        .channels = 1,
        .bits = 16,
    };
    esp_gmf_pipeline_report_info(pipe, ESP_GMF_INFO_SOUND, &info, sizeof(info));

    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_pipeline_run(pipe));

    vTaskDelay(10000 / portTICK_RATE_MS);
    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_pipeline_stop(pipe));

    esp_gmf_task_deinit(work_task);
    esp_gmf_pipeline_destroy(pipe);
    pool_unregister_audio_codecs();
    esp_gmf_pool_deinit(pool);
#ifdef MEDIA_LIB_MEM_TEST
    media_lib_stop_mem_trace();
#endif /* MEDIA_LIB_MEM_TEST */
    esp_gmf_teardown_periph_codec(NULL, record_dev);
    esp_gmf_teardown_periph_sdmmc(sdcard);
    esp_gmf_teardown_periph_i2c(0);
    vTaskDelay(1000 / portTICK_RATE_MS);
    ESP_GMF_MEM_SHOW(TAG);
}

TEST_CASE("Recorder, One Pipe recoding multiple format, [IIS->ENC->FILE]", "ESP_GMF_POOL")
{
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("ESP_GMF_PIPELINE", ESP_LOG_DEBUG);
    esp_log_level_set("ESP_GMF_POOL", ESP_LOG_DEBUG);
    ESP_GMF_MEM_SHOW(TAG);

    void *sdcard = NULL;
    esp_gmf_setup_periph_sdmmc(&sdcard);
    esp_gmf_setup_periph_i2c(0);
    esp_gmf_setup_periph_aud_info rec_info = {
        .sample_rate = 16000,
        .channel = 1,
        .bits_per_sample = 16,
        .port_num = 0,
    };
    void *record_dev = NULL;
    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_setup_periph_codec(NULL, &rec_info, NULL, &record_dev));
#ifdef MEDIA_LIB_MEM_TEST
    media_lib_add_default_adapter();
#endif /* MEDIA_LIB_MEM_TEST */
    esp_gmf_pool_handle_t pool = NULL;
    esp_gmf_pool_init(&pool);
    TEST_ASSERT_NOT_NULL(pool);
    pool_register_audio_codecs(pool);
    pool_register_audio_effects(pool);
    pool_register_io(pool);
    pool_register_codec_dev_io(pool, NULL, record_dev);
    ESP_GMF_POOL_SHOW_ITEMS(pool);

    esp_gmf_pipeline_handle_t pipe = NULL;
    const char *name[] = {"rate_cvt", "encoder"};
    esp_gmf_pool_new_pipeline(pool, "codec_dev_rx", name, sizeof(name) / sizeof(char *), "file", &pipe);
    TEST_ASSERT_NOT_NULL(pipe);

    esp_gmf_task_cfg_t cfg = DEFAULT_ESP_GMF_TASK_CONFIG();
    cfg.ctx = NULL;
    cfg.cb = NULL;
    cfg.thread.stack = 40 * 1024;
    esp_gmf_task_handle_t work_task = NULL;
    esp_gmf_task_init(&cfg, &work_task);
    TEST_ASSERT_NOT_NULL(work_task);

    esp_gmf_pipeline_bind_task(pipe, work_task);
    esp_gmf_pipeline_loading_jobs(pipe);
    esp_gmf_pipeline_set_event(pipe, _pipeline_event, NULL);

    char uri[128] = {0};
    uint32_t I2S_REC_SAMPLE_RATE = 16000;
    uint32_t I2S_REC_ENCODER_SAMPLE_RATE = 48000;

    for (int i = 0; i < sizeof(test_enc_format) / sizeof(char *); ++i) {
        snprintf(uri, 127, "/sdcard/esp_gmf_rec_%02d.%s", i, test_enc_format[i]);
        esp_gmf_element_handle_t enc_handle = NULL;
        esp_gmf_pipeline_get_el_by_name(pipe, "encoder", &enc_handle);
        esp_audio_enc_config_t *esp_gmf_enc_cfg = (esp_audio_enc_config_t *)OBJ_GET_CFG(enc_handle);
        esp_audio_type_t audio_type = 0;
        esp_gmf_info_sound_t info = {
            .sample_rates = I2S_REC_SAMPLE_RATE,
            .channels = 1,
            .bits = 16,
        };
        esp_gmf_audio_helper_get_audio_type_by_uri(uri, &audio_type);
        esp_gmf_audio_helper_reconfig_enc_by_type (audio_type, &info, esp_gmf_enc_cfg);
        esp_gmf_pipeline_report_info(pipe, ESP_GMF_INFO_SOUND, &info, sizeof(info));

        esp_gmf_element_handle_t resp = NULL;
        esp_gmf_pipeline_get_el_by_name(pipe, "rate_cvt", &resp);
        esp_ae_rate_cvt_cfg_t *resp_cfg = (esp_ae_rate_cvt_cfg_t *)OBJ_GET_CFG(resp);
        resp_cfg->src_rate = I2S_REC_SAMPLE_RATE;
        if (audio_type == ESP_AUDIO_TYPE_AMRNB) {
            resp_cfg->dest_rate = 8000;
        } else if (audio_type == ESP_AUDIO_TYPE_G711U) {
            resp_cfg->dest_rate = 8000;
        } else if (audio_type == ESP_AUDIO_TYPE_G711A) {
            resp_cfg->dest_rate = 8000;
        } else if (audio_type == ESP_AUDIO_TYPE_AMRWB) {
            resp_cfg->dest_rate = 16000;
        } else {
            resp_cfg->dest_rate = I2S_REC_ENCODER_SAMPLE_RATE;
        }

        ESP_LOGW(TAG, "Recoding file, %s, type:%d\r\n", uri, esp_gmf_enc_cfg->type);
        TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_pipeline_set_out_uri(pipe, uri));
        TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_pipeline_run(pipe));
        int cnt = 10;
        while (cnt) {
            vTaskDelay(1000 / portTICK_RATE_MS);
            printf(".");
            fflush(stdout);
            cnt--;
        }
        TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_pipeline_stop(pipe));
        TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_pipeline_reset(pipe));
        TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_pipeline_loading_jobs(pipe));
    }

    esp_gmf_task_deinit(work_task);
    esp_gmf_pipeline_destroy(pipe);
    pool_unregister_audio_codecs();
    esp_gmf_pool_deinit(pool);
#ifdef MEDIA_LIB_MEM_TEST
    media_lib_stop_mem_trace();
#endif /* MEDIA_LIB_MEM_TEST */
    esp_gmf_teardown_periph_codec(NULL, record_dev);
    esp_gmf_teardown_periph_sdmmc(sdcard);
    esp_gmf_teardown_periph_i2c(0);
    vTaskDelay(1000 / portTICK_RATE_MS);
    ESP_GMF_MEM_SHOW(TAG);
}

TEST_CASE("Recorder, One Pipe, [IIS->ENC->HTTP]", "ESP_GMF_POOL")
{
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("ESP_GMF_PIPELINE", ESP_LOG_DEBUG);
    esp_log_level_set("ESP_GMF_POOL", ESP_LOG_DEBUG);
    ESP_GMF_MEM_SHOW(TAG);
    esp_gmf_setup_periph_wifi();
    esp_gmf_setup_periph_i2c(0);
    esp_gmf_setup_periph_aud_info rec_info = {
        .sample_rate = 16000,
        .channel = 1,
        .bits_per_sample = 16,
        .port_num = 0,
    };
    void *record_dev = NULL;
    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_setup_periph_codec(NULL, &rec_info, NULL, &record_dev));

#ifdef MEDIA_LIB_MEM_TEST
    media_lib_add_default_adapter();
#endif /* MEDIA_LIB_MEM_TEST */
    esp_gmf_pool_handle_t pool = NULL;
    esp_gmf_pool_init(&pool);
    TEST_ASSERT_NOT_NULL(pool);
    pool_register_audio_codecs(pool);
    pool_register_audio_effects(pool);
    pool_register_io(pool);
    pool_register_codec_dev_io(pool, NULL, record_dev);
    ESP_GMF_POOL_SHOW_ITEMS(pool);

    // Create the new elements
    esp_gmf_pipeline_handle_t pipe = NULL;
    const char *name[] = {"encoder"};
    esp_gmf_pool_new_pipeline(pool, "codec_dev_rx", name, sizeof(name) / sizeof(char *), "http", &pipe);
    TEST_ASSERT_NOT_NULL(pipe);

    const char *rec_type = ".aac";

    esp_gmf_element_handle_t enc_handle = NULL;
    esp_gmf_pipeline_get_el_by_name(pipe, "encoder", &enc_handle);
    esp_audio_enc_config_t *esp_gmf_enc_cfg = (esp_audio_enc_config_t *)OBJ_GET_CFG(enc_handle);
    esp_audio_type_t audio_type = 0;
    esp_gmf_audio_helper_get_audio_type_by_uri(rec_type, &audio_type);
    esp_gmf_info_sound_t info = {
        .sample_rates = 16000,
        .channels = 1,
        .bits = 16,
    };
    esp_gmf_audio_helper_reconfig_enc_by_type (audio_type, &info, esp_gmf_enc_cfg);
    esp_gmf_pipeline_report_info(pipe, ESP_GMF_INFO_SOUND, &info, sizeof(info));

    esp_gmf_io_handle_t http_out = NULL;
    esp_gmf_pipeline_get_out(pipe, &http_out);
    http_io_cfg_t *http_cfg = (http_io_cfg_t *)OBJ_GET_CFG(http_out);
    http_cfg->user_data = (void *)rec_type;

    esp_gmf_task_cfg_t cfg = DEFAULT_ESP_GMF_TASK_CONFIG();
    cfg.ctx = NULL;
    cfg.cb = NULL;
    esp_gmf_task_handle_t work_task = NULL;
    esp_gmf_task_init(&cfg, &work_task);
    TEST_ASSERT_NOT_NULL(work_task);

    esp_gmf_pipeline_bind_task(pipe, work_task);
    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_pipeline_loading_jobs(pipe));
    esp_gmf_pipeline_set_event(pipe, _pipeline_event, NULL);
    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_pipeline_set_out_uri(pipe, "http://192.168.31.28:8000/upload"));
    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_pipeline_run(pipe));

    vTaskDelay(15000 / portTICK_RATE_MS);
    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_pipeline_stop(pipe));
    esp_gmf_task_deinit(work_task);
    esp_gmf_pipeline_destroy(pipe);
    pool_unregister_audio_codecs();
    esp_gmf_pool_deinit(pool);
#ifdef MEDIA_LIB_MEM_TEST
    media_lib_stop_mem_trace();
#endif /* MEDIA_LIB_MEM_TEST */
    esp_gmf_teardown_periph_codec(NULL, record_dev);
    esp_gmf_teardown_periph_wifi();
    esp_gmf_teardown_periph_i2c(0);
    vTaskDelay(1000 / portTICK_RATE_MS);
    ESP_GMF_MEM_SHOW(TAG);
}
