/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2024 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
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

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_gmf_oal_mem.h"
#include "esp_gmf_data_bus.h"

static const char *TAG = "ESP_GMF_DATA_BUS";

esp_gmf_err_t esp_gmf_db_init(esp_gmf_db_config_t *db_config, esp_gmf_db_handle_t *hd)
{
    ESP_GMF_NULL_CHECK(TAG, db_config, return ESP_GMF_ERR_INVALID_ARG;);
    ESP_GMF_NULL_CHECK(TAG, hd, return ESP_GMF_ERR_INVALID_ARG;);
    esp_gmf_data_bus_t *db = esp_gmf_oal_calloc(1, sizeof(esp_gmf_data_bus_t));
    ESP_GMF_NULL_CHECK(TAG, db, return ESP_GMF_ERR_MEMORY_LACK;);
    db->name = esp_gmf_oal_strdup(db_config->name == NULL ? "NULL" : db_config->name);
    ESP_GMF_MEM_CHECK(TAG, db->name, goto __init_fail);
    db->type = db_config->type;
    db->max_item_num = db_config->max_item_num;
    db->max_size = db_config->max_size;
    db->child = db_config->child;
    *hd = db;
    return ESP_GMF_ERR_OK;

__init_fail:
    if (db) {
        esp_gmf_oal_free(db);
        db = NULL;
    }
    return ESP_GMF_ERR_FAIL;
}

esp_gmf_err_t esp_gmf_db_deinit(esp_gmf_db_handle_t handle)
{
    ESP_GMF_NULL_CHECK(TAG, handle, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_data_bus_t *db = (esp_gmf_data_bus_t *)handle;
    if (db->op.deinit) {
        db->op.deinit(db->child);
    }
    if (db->name) {
        esp_gmf_oal_free((void *)db->name);
        db->name = NULL;
    }
    if (db) {
        esp_gmf_oal_free(db);
        db = NULL;
    }
    return ESP_GMF_ERR_OK;
}

esp_gmf_err_t esp_gmf_db_read(esp_gmf_db_handle_t handle, void *buffer, int buf_len, int block_ticks)
{
    ESP_GMF_NULL_CHECK(TAG, handle, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_data_bus_t *db = (esp_gmf_data_bus_t *)handle;
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
    if (db->op.read) {
        ret = db->op.read(db->child, buffer, buf_len, block_ticks);
    }
    return ret;
}

esp_gmf_err_t esp_gmf_db_write(esp_gmf_db_handle_t handle, void *buffer, int buf_len, int block_ticks)
{
    ESP_GMF_NULL_CHECK(TAG, handle, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_data_bus_t *db = (esp_gmf_data_bus_t *)handle;
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
    if (db->op.write) {
        ret = db->op.write(db->child, buffer, buf_len, block_ticks);
    }
    return ret;
}

esp_gmf_err_io_t esp_gmf_db_acquire_read(esp_gmf_db_handle_t handle, esp_gmf_data_bus_block_t *blk, uint32_t wanted_size, int block_ticks)
{
    ESP_GMF_NULL_CHECK(TAG, handle, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_data_bus_t *db = (esp_gmf_data_bus_t *)handle;
    esp_gmf_err_io_t ret = ESP_GMF_ERR_OK;
    if (db->op.acquire_read) {
        ret = db->op.acquire_read(db->child, blk, wanted_size, block_ticks);
    }
    return ret;
}

esp_gmf_err_io_t esp_gmf_db_release_read(esp_gmf_db_handle_t handle, esp_gmf_data_bus_block_t *blk, int block_ticks)
{
    ESP_GMF_NULL_CHECK(TAG, handle, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_data_bus_t *db = (esp_gmf_data_bus_t *)handle;
    esp_gmf_err_io_t ret = ESP_GMF_ERR_OK;
    if (db->op.release_read) {
        ret = db->op.release_read(db->child, blk, block_ticks);
    }
    return ret;
}

esp_gmf_err_io_t esp_gmf_db_acquire_write(esp_gmf_db_handle_t handle, esp_gmf_data_bus_block_t *blk, uint32_t wanted_size, int block_ticks)
{
    ESP_GMF_NULL_CHECK(TAG, handle, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_data_bus_t *db = (esp_gmf_data_bus_t *)handle;
    esp_gmf_err_io_t ret = ESP_GMF_ERR_OK;
    if (db->op.acquire_write) {
        ret = db->op.acquire_write(db->child, blk, wanted_size, block_ticks);
    }
    return ret;
}

esp_gmf_err_io_t esp_gmf_db_release_write(esp_gmf_db_handle_t handle, esp_gmf_data_bus_block_t *blk, int block_ticks)
{
    ESP_GMF_NULL_CHECK(TAG, handle, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_data_bus_t *db = (esp_gmf_data_bus_t *)handle;
    esp_gmf_err_io_t ret = ESP_GMF_ERR_OK;
    if (db->op.release_write) {
        ret = db->op.release_write(db->child, blk, block_ticks);
    }
    return ret;
}

esp_gmf_err_t esp_gmf_db_done_write(esp_gmf_db_handle_t handle)
{
    ESP_GMF_NULL_CHECK(TAG, handle, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_data_bus_t *db = (esp_gmf_data_bus_t *)handle;
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
    if (db->op.done_write) {
        ret = db->op.done_write(db->child);
    }
    return ret;
}

esp_gmf_err_t esp_gmf_db_reset_done_write(esp_gmf_db_handle_t handle)
{
    ESP_GMF_NULL_CHECK(TAG, handle, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_data_bus_t *db = (esp_gmf_data_bus_t *)handle;
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
    if (db->op.reset_done_write) {
        ret = db->op.reset_done_write(db->child);
    }
    return ret;
}

esp_gmf_err_t esp_gmf_db_reset(esp_gmf_db_handle_t handle)
{
    ESP_GMF_NULL_CHECK(TAG, handle, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_data_bus_t *db = (esp_gmf_data_bus_t *)handle;
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
    if (db->op.reset) {
        ret = db->op.reset(db->child);
    }
    return ret;
}

esp_gmf_err_t esp_gmf_db_abort(esp_gmf_db_handle_t handle)
{
    ESP_GMF_NULL_CHECK(TAG, handle, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_data_bus_t *db = (esp_gmf_data_bus_t *)handle;
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
    if (db->op.abort) {
        ret = db->op.abort(db->child);
    }
    return ret;
}

esp_gmf_err_t esp_gmf_db_get_total_size(esp_gmf_db_handle_t handle, uint32_t *buff_size)
{
    ESP_GMF_NULL_CHECK(TAG, handle, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_data_bus_t *db = (esp_gmf_data_bus_t *)handle;
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
    if (db->op.get_total_size) {
        ret = db->op.get_total_size(db->child, buff_size);
    }
    return ret;
}

esp_gmf_err_t esp_gmf_db_get_filled_size(esp_gmf_db_handle_t handle, uint32_t *filled_size)
{
    ESP_GMF_NULL_CHECK(TAG, handle, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_data_bus_t *db = (esp_gmf_data_bus_t *)handle;
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
    if (db->op.get_filled_size) {
        ret = db->op.get_filled_size(db->child, filled_size);
    }
    return ret;
}

esp_gmf_err_t esp_gmf_db_get_available(esp_gmf_db_handle_t handle, uint32_t *available_size)
{
    ESP_GMF_NULL_CHECK(TAG, handle, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_data_bus_t *db = (esp_gmf_data_bus_t *)handle;
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
    if (db->op.get_available) {
        ret = db->op.get_available(db->child, available_size);
    }
    return ret;
}

esp_gmf_err_t esp_gmf_db_set_writer(esp_gmf_db_handle_t handle, void *holder)
{
    ESP_GMF_NULL_CHECK(TAG, handle, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_data_bus_t *db = (esp_gmf_data_bus_t *)handle;
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
    db->writer = holder;
    return ret;
}

esp_gmf_err_t esp_gmf_db_get_writer(esp_gmf_db_handle_t handle, void **holder)
{
    ESP_GMF_NULL_CHECK(TAG, handle, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_data_bus_t *db = (esp_gmf_data_bus_t *)handle;
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
    if (holder) {
        *holder = db->writer;
    }
    return ret;
}

esp_gmf_err_t esp_gmf_db_set_reader(esp_gmf_db_handle_t handle, void *holder)
{
    ESP_GMF_NULL_CHECK(TAG, handle, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_data_bus_t *db = (esp_gmf_data_bus_t *)handle;
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
    db->reader = holder;
    return ret;
}

esp_gmf_err_t esp_gmf_db_get_reader(esp_gmf_db_handle_t handle, void **holder)
{
    ESP_GMF_NULL_CHECK(TAG, handle, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_data_bus_t *db = (esp_gmf_data_bus_t *)handle;
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
    if (holder) {
        *holder = db->reader;
    }
    return ret;
}

esp_gmf_err_t esp_gmf_db_get_type(esp_gmf_db_handle_t handle, esp_gmf_data_bus_type_t *db_type)
{
    ESP_GMF_NULL_CHECK(TAG, handle, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_data_bus_t *db = (esp_gmf_data_bus_t *)handle;
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
    *db_type = db->type;
    return ret;
}

const char *esp_gmf_db_get_name(esp_gmf_db_handle_t handle)
{
    ESP_GMF_NULL_CHECK(TAG, handle, return NULL);
    esp_gmf_data_bus_t *db = (esp_gmf_data_bus_t *)handle;
    return db->name;
}
