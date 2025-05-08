/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "unity.h"
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_err.h"

#include "esp_gmf_element.h"
#include "esp_gmf_pipeline.h"
#include "esp_gmf_pool.h"
#include "esp_gmf_oal_mem.h"
#include "esp_gmf_oal_thread.h"

#include "esp_gmf_audio_dec.h"
#include "esp_audio_dec_default.h"
#include "esp_audio_dec_reg.h"
#include "esp_gmf_new_databus.h"
#include "esp_gmf_setup_peripheral.h"
#include "esp_gmf_setup_pool.h"
#include "esp_gmf_audio_helper.h"

static const char *TAG = "GMF_AUDIO_PLAY_COM";

void play_pause_single_file( esp_gmf_pipeline_handle_t pipe, const char *uri)
{
    ESP_LOGW(TAG, "play_single_file: %s\r\n", uri);
    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_pipeline_reset(pipe));
    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_pipeline_loading_jobs(pipe));
    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_pipeline_set_in_uri(pipe, uri));

    esp_gmf_element_handle_t dec_el = NULL;
    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_pipeline_get_el_by_name(pipe, "aud_simp_dec", &dec_el));
    esp_audio_simple_dec_cfg_t *simple_cfg = (esp_audio_simple_dec_cfg_t *)OBJ_GET_CFG(dec_el);
    TEST_ASSERT_NOT_NULL(simple_cfg);
    esp_gmf_info_sound_t info = {
        .sample_rates = 48000,
        .channels = 2,
        .bits = 16,
    };
    esp_gmf_audio_helper_reconfig_dec_by_uri(uri, &info, simple_cfg);
    ESP_GMF_MEM_SHOW(TAG);
    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_pipeline_run(pipe));

    vTaskDelay(2000 / portTICK_RATE_MS);
    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_pipeline_pause(pipe));
    vTaskDelay(1000 / portTICK_RATE_MS);
    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_pipeline_resume(pipe));
}
