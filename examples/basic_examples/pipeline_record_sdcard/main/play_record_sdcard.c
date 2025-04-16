/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_gmf_element.h"
#include "esp_gmf_pipeline.h"
#include "esp_gmf_pool.h"
#include "esp_gmf_audio_enc.h"
#include "esp_gmf_audio_helper.h"
#include "esp_gmf_setup_peripheral.h"
#include "esp_gmf_setup_pool.h"
#include "esp_gmf_io_codec_dev.h"

static const char *TAG = "REC_SDCARD";

esp_err_t _pipeline_event(esp_gmf_event_pkt_t *event, void *ctx)
{
    ESP_LOGI(TAG, "CB: RECV Pipeline EVT: el:%s-%p, type:%d, sub:%s, payload:%p, size:%d,%p",
             "OBJ_GET_TAG(event->from)", event->from, event->type, esp_gmf_event_get_state_str(event->sub),
             event->payload, event->payload_size, ctx);
    return 0;
}

void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);
    int ret;
    ESP_LOGI(TAG, "[ 1 ] Mount sdcard");
    void *card = NULL;
    esp_gmf_setup_periph_sdmmc(&card);
    esp_gmf_setup_periph_i2c(0);
    esp_gmf_setup_periph_aud_info record_info = {
        .sample_rate = 16000,
        .channel = 1,
        .bits_per_sample = 16,
        .port_num = 0,
    };
    void *play_dev = NULL;
    void *record_dev = NULL;
#ifdef CONFIG_IDF_TARGET_ESP32
    esp_gmf_setup_periph_aud_info play_info = {0};
    memcpy(&play_info, &record_info, sizeof(esp_gmf_setup_periph_aud_info));
    record_info.port_num = 1;
    ret = esp_gmf_setup_periph_codec(&play_info, &record_info, &play_dev, &record_dev);
#else
    ret = esp_gmf_setup_periph_codec(NULL, &record_info, NULL, &record_dev);
#endif
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, { return; }, "Failed to setup rec codec");
    ESP_LOGI(TAG, "[ 2 ] Register all the elements and set audio information to record codec device");
    esp_gmf_pool_handle_t pool = NULL;
    esp_gmf_pool_init(&pool);
    pool_register_codec_dev_io(pool, play_dev, record_dev);
    pool_register_io(pool);
    pool_register_audio_codecs(pool);
    pool_register_audio_effects(pool);
    ESP_GMF_POOL_SHOW_ITEMS(pool);

    ESP_LOGI(TAG, "[ 3 ] Create audio pipeline");
    esp_gmf_pipeline_handle_t pipe = NULL;
    const char *name[] = {"encoder"};
    ret = esp_gmf_pool_new_pipeline(pool, "codec_dev_rx", name, sizeof(name) / sizeof(char *), "file", &pipe);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, { return; }, "Failed to new pipeline");

    ESP_LOGI(TAG, "[ 3.1 ] Set audio url to record");
    esp_gmf_pipeline_set_out_uri(pipe, "/sdcard/esp_gmf_rec001.aac");

    ESP_LOGI(TAG, "[ 3.2 ] Reconfig audio encoder type by url and audio information and report information to the record pipeline");
    esp_gmf_element_handle_t enc_el = NULL;
    esp_gmf_pipeline_get_el_by_name(pipe, "encoder", &enc_el);
    esp_gmf_info_sound_t info = {
        .sample_rates = 16000,
        .channels = 1,
        .bits = 16,
    };
    esp_gmf_audio_helper_reconfig_enc_by_type(ESP_AUDIO_TYPE_AAC, &info, (esp_audio_enc_config_t *)OBJ_GET_CFG(enc_el));
    esp_gmf_pipeline_report_info(pipe, ESP_GMF_INFO_SOUND, &info, sizeof(info));

    ESP_LOGI(TAG, "[ 3.3 ] Create gmf task, bind task to pipeline and load linked element jobs to the bind task");
    esp_gmf_task_cfg_t cfg = DEFAULT_ESP_GMF_TASK_CONFIG();
    cfg.ctx = NULL;
    cfg.cb = NULL;
    esp_gmf_task_handle_t work_task = NULL;
    ret = esp_gmf_task_init(&cfg, &work_task);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, { return; }, "Failed to create pipeline task");
    esp_gmf_pipeline_bind_task(pipe, work_task);
    esp_gmf_pipeline_loading_jobs(pipe);

    ESP_LOGI(TAG, "[ 3.4 ] Create envent group and listening event from pipeline");
    esp_gmf_pipeline_set_event(pipe, _pipeline_event, NULL);

    ESP_LOGI(TAG, "[ 4 ] Start audio_pipeline");
    esp_gmf_pipeline_run(pipe);

    ESP_LOGI(TAG, "[ 5 ] Wait for a while to stop record pipeline");
    vTaskDelay(10000 / portTICK_PERIOD_MS);
    esp_gmf_pipeline_stop(pipe);

    ESP_LOGI(TAG, "[ 6 ] Destroy all the resources");
    esp_gmf_task_deinit(work_task);
    esp_gmf_pipeline_destroy(pipe);
    esp_gmf_pool_deinit(pool);
    pool_unregister_audio_codecs();
    esp_gmf_teardown_periph_codec(play_dev, record_dev);
    esp_gmf_teardown_periph_sdmmc(card);
    esp_gmf_teardown_periph_i2c(0);
}
