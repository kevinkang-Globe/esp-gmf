/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <string.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_gmf_element.h"
#include "esp_gmf_pipeline.h"
#include "esp_gmf_pool.h"
#include "esp_embed_tone.h"
#include "esp_gmf_io_embed_flash.h"
#include "esp_gmf_setup_peripheral.h"
#include "esp_gmf_setup_pool.h"
#include "esp_gmf_audio_helper.h"

static const char *TAG = "PLAY_EMBED_MUSIC";

#define PIPELINE_BLOCK_BIT BIT(0)

esp_err_t _pipeline_event(esp_gmf_event_pkt_t *event, void *ctx)
{
    ESP_LOGI(TAG, "CB: RECV Pipeline EVT: el: %s-%p, type: %x, sub: %s, payload: %p, size: %d, %p",
             "OBJ_GET_TAG(event->from)", event->from, event->type, esp_gmf_event_get_state_str(event->sub),
             event->payload, event->payload_size, ctx);
    if ((event->sub == ESP_GMF_EVENT_STATE_STOPPED)
        || (event->sub == ESP_GMF_EVENT_STATE_FINISHED)
        || (event->sub == ESP_GMF_EVENT_STATE_ERROR)) {
        xEventGroupSetBits((EventGroupHandle_t)ctx, PIPELINE_BLOCK_BIT);
    }
    return 0;
}

void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);
    ESP_GMF_MEM_SHOW(TAG);
    int ret;
    ESP_LOGI(TAG, "[ 1 ] Mount peripheral");
#ifndef CONFIG_IDF_TARGET_ESP32C3
    esp_gmf_setup_periph_i2c(0);
#endif /* CONFIG_IDF_TARGET_ESP32C3 */
    esp_gmf_setup_periph_aud_info play_info = {
        .sample_rate = 48000,
        .channel = 2,
        .bits_per_sample = 16,
        .port_num = 0,
    };
    void *play_dev = NULL;
    void *record_dev = NULL;
#ifdef CONFIG_IDF_TARGET_ESP32
    esp_gmf_setup_periph_aud_info record_info = {0};
    memcpy(&record_info, &play_info, sizeof(esp_gmf_setup_periph_aud_info));
    record_info.port_num = 1;
    ret = esp_gmf_setup_periph_codec(&play_info, &record_info, &play_dev, &record_dev);
#else
    ret = esp_gmf_setup_periph_codec(&play_info, NULL, &play_dev, NULL);
#endif
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, { return; }, "Failed to setup play codec");

    ESP_LOGI(TAG, "[ 2 ] Register all the elements and set audio information to play codec device");
    esp_gmf_pool_handle_t pool = NULL;
    esp_gmf_pool_init(&pool);
    pool_register_io(pool);
    pool_register_audio_codecs(pool);
    pool_register_audio_effects(pool);
    pool_register_codec_dev_io(pool, play_dev, record_dev);
    ESP_GMF_POOL_SHOW_ITEMS(pool);

    ESP_LOGI(TAG, "[ 3 ] Create audio pipeline");
    esp_gmf_pipeline_handle_t pipe = NULL;
    const char *name[] = {"aud_simp_dec", "bit_cvt", "rate_cvt", "ch_cvt"};
    ret = esp_gmf_pool_new_pipeline(pool, "embed", name, sizeof(name) / sizeof(char *), "codec_dev_tx", &pipe);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, { return; }, "Failed to new pipeline");

    ESP_LOGI(TAG, "[ 3.1 ] Set audio url to play");
    esp_gmf_pipeline_set_in_uri(pipe, esp_embed_tone_url[ESP_EMBED_TONE_FF_16B_1C_44100HZ_MP3]);
    esp_gmf_io_handle_t in_io = NULL;
    esp_gmf_pipeline_get_in(pipe, &in_io);
    esp_gmf_io_embed_flash_set_context(in_io, (embed_item_info_t *)&g_esp_embed_tone[0], ESP_EMBED_TONE_URL_MAX);
    esp_gmf_element_handle_t dec_el = NULL;
    esp_gmf_pipeline_get_el_by_name(pipe, "aud_simp_dec", &dec_el);
    esp_gmf_info_sound_t info = {0};
    esp_gmf_audio_helper_reconfig_dec_by_uri(esp_embed_tone_url[ESP_EMBED_TONE_FF_16B_1C_44100HZ_MP3], &info, OBJ_GET_CFG(dec_el));

    ESP_LOGI(TAG, "[ 3.2 ] Create gmf task, bind task to pipeline and load linked element jobs to the bind task");
    esp_gmf_task_cfg_t cfg = DEFAULT_ESP_GMF_TASK_CONFIG();
    cfg.ctx = NULL;
    cfg.cb = NULL;
    esp_gmf_task_handle_t work_task = NULL;
    ret = esp_gmf_task_init(&cfg, &work_task);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, { return; }, "Failed to create pipeline task");
    esp_gmf_pipeline_bind_task(pipe, work_task);
    esp_gmf_pipeline_loading_jobs(pipe);

    ESP_LOGI(TAG, "[ 3.3 ] Create envent group and listening event from pipeline");
    EventGroupHandle_t pipe_sync_evt = xEventGroupCreate();
    ESP_GMF_NULL_CHECK(TAG, pipe_sync_evt, return);
    esp_gmf_pipeline_set_event(pipe, _pipeline_event, pipe_sync_evt);

    ESP_LOGI(TAG, "[ 4 ] Start audio_pipeline");
    esp_gmf_pipeline_run(pipe);

    // Wait to finished or got error
    ESP_LOGI(TAG, "[ 5 ] Wait stop event to the pipeline and stop all the pipeline");
    xEventGroupWaitBits(pipe_sync_evt, PIPELINE_BLOCK_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
    esp_gmf_pipeline_stop(pipe);

    ESP_LOGI(TAG, "[ 6 ] Destroy all the resources");
    pool_unregister_audio_codecs();
    esp_gmf_task_deinit(work_task);
    esp_gmf_pipeline_destroy(pipe);
    esp_gmf_pool_deinit(pool);
    esp_gmf_teardown_periph_codec(play_dev, record_dev);
#ifndef CONFIG_IDF_TARGET_ESP32C3
    esp_gmf_teardown_periph_i2c(0);
#endif /* CONFIG_IDF_TARGET_ESP32C3 */
    ESP_GMF_MEM_SHOW(TAG);
}
