/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "unity.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_gmf_data_bus.h"
#include "esp_gmf_video_scale.h"
#include "esp_gmf_video_crop.h"
#include "esp_gmf_video_rotate.h"
#include "esp_gmf_video_color_convert.h"

#define TEST_MAX_EL_NUM     (4)
#define TEST_MAX_IMG_LENGTH (640 * 480 * 3)
#define TEST_TICK_DELAY     (0)
static esp_gmf_obj_handle_t obj_hd_pool[TEST_MAX_EL_NUM] = {NULL};
static uint8_t             *test_buffer                  = NULL;
static uint8_t             *test_buffer1                 = NULL;

static const char *TAG = "IMGFX_SCALE_TEST";

void esp_gmf_destory_obj_cfg_pool()
{
    for (size_t i = 0; i < TEST_MAX_EL_NUM; i++) {
        esp_gmf_obj_delete(obj_hd_pool[i]);
        obj_hd_pool[i] = NULL;
    }
}

static esp_gmf_err_t esp_gmf_create_obj_cfg_pool()
{
    esp_gmf_job_err_t ret = ESP_GMF_JOB_ERR_OK;
    esp_imgfx_scale_cfg_t scale_cfg = {
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
        .in_res = {640, 480},
        .scale_res = {320, 240},
        .filter_type = ESP_IMGFX_SCALE_FILTER_TYPE_BILINEAR};
    ret = esp_gmf_video_scale_init(&scale_cfg, &obj_hd_pool[0]);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, { goto _cfg_free;}, "Failed to append scale obj to pool");
    esp_imgfx_crop_cfg_t crop_cfg = {
        .in_res = {100, 100},
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
        .cropped_res = {50, 50},
        .x_pos = 25,
        .y_pos = 25};
    ret = esp_gmf_video_crop_init(&crop_cfg, &obj_hd_pool[1]);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, { goto _cfg_free;}, "Failed to append crop obj to pool");
    esp_imgfx_rotate_cfg_t rotate_cfg = {
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
        .in_res = {.width = 256, .height = 256},
        .degree = 90};
    ret = esp_gmf_video_rotate_init(&rotate_cfg, &obj_hd_pool[2]);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, { goto _cfg_free;}, "Failed to append rotate obj to pool");
    esp_imgfx_color_convert_cfg_t color_convert_cfg = {
        .in_res = {640, 480},
        .in_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB565_LE,
        .out_pixel_fmt = ESP_IMGFX_PIXEL_FMT_RGB888,
        .color_space_std = ESP_IMGFX_COLOR_SPACE_STD_BT601};
    ret = esp_gmf_video_color_convert_init(&color_convert_cfg, &obj_hd_pool[3]);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, { goto _cfg_free;}, "Failed to append color convert obj to pool");
    return ret;
_cfg_free:
    esp_gmf_destory_obj_cfg_pool();
    return ret;
}

static int imgfx_acquire_read(void *handle, esp_gmf_data_bus_block_t *blk, int wanted_size, int block_ticks)
{
    blk->buf = test_buffer;
    blk->buf_length = TEST_MAX_IMG_LENGTH;
    blk->valid_size = wanted_size;
    memset(blk->buf, 1, wanted_size);
    return ESP_OK;
}

static int imgfx_release_read(void *handle, esp_gmf_data_bus_block_t *blk, int block_ticks)
{
    return ESP_OK;
}

static int imgfx_acquire_write(void *handle, esp_gmf_data_bus_block_t *blk, int wanted_size, int block_ticks)
{
    blk->buf = test_buffer1;
    blk->buf_length = TEST_MAX_IMG_LENGTH;
    return ESP_OK;
}

static int imgfx_release_write(void *handle, esp_gmf_data_bus_block_t *blk, int block_ticks)
{
    ESP_LOGI(TAG, "Output size %d", blk->valid_size);
    return ESP_OK;
}

TEST_CASE("Test for all software video effects", "[ESP_GMF_Effects]")
{
    esp_log_level_set("*", ESP_LOG_INFO);
    ESP_GMF_MEM_SHOW(TAG);
    esp_gmf_port_handle_t in_port = NULL;
    esp_gmf_port_handle_t out_port = NULL;
    esp_gmf_obj_handle_t obj_hd = {NULL};
    // Create obj handle and cfg pool
    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_create_obj_cfg_pool());
    test_buffer = (uint8_t *)malloc(TEST_MAX_IMG_LENGTH);
    TEST_ASSERT_NOT_EQUAL(NULL, test_buffer);
    test_buffer1 = (uint8_t *)malloc(TEST_MAX_IMG_LENGTH);
    TEST_ASSERT_NOT_EQUAL(NULL, test_buffer1);

    for (size_t test_count = 0; test_count < 5; test_count++) {
        for (size_t el_index = 0; el_index < TEST_MAX_EL_NUM; el_index++) {
            // New obj
            TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_obj_dupl(obj_hd_pool[el_index], &obj_hd));
            // Print obj tag
            ESP_LOGE(TAG, "%s-%d,obj_hd:%p", (OBJ_GET_TAG(obj_hd)), __LINE__, obj_hd);
            // Create in/out port
            in_port = NEW_ESP_GMF_PORT_IN_BLOCK(imgfx_acquire_read, imgfx_release_read, NULL, NULL, TEST_MAX_IMG_LENGTH, TEST_TICK_DELAY);
            out_port = NEW_ESP_GMF_PORT_OUT_BLOCK(imgfx_acquire_write, imgfx_release_write, NULL, NULL, TEST_MAX_IMG_LENGTH, TEST_TICK_DELAY);
            // Register in/out port
            esp_gmf_element_register_in_port(obj_hd, in_port);
            esp_gmf_element_register_out_port(obj_hd, out_port);
            for (size_t i = 0; i < 4; i++) {
                // Open obj
                TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_element_process_open(obj_hd, NULL));
                // Run obj
                TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_element_process_running(obj_hd, NULL));
                TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_element_process_running(obj_hd, NULL));
                // Close obj
                TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_element_process_close(obj_hd, NULL));
            }
            // Delete obj: port will be deleted when obj handle pool is destroyed
            TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_obj_delete(obj_hd));
        }
    }
    // free buffer
    free(test_buffer);
    free(test_buffer1);
    // Destroy obj handle and cfg pool
    esp_gmf_destory_obj_cfg_pool();
    ESP_GMF_MEM_SHOW(TAG);
}
