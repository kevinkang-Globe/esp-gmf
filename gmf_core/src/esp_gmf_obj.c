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

#include <stdlib.h>
#include <string.h>
#include "esp_gmf_obj.h"
#include "esp_gmf_err.h"
#include "esp_log.h"
#include "esp_gmf_oal_mem.h"

static const char *TAG = "ESP_GMF_OBJ";

esp_gmf_err_t esp_gmf_obj_dupl(esp_gmf_obj_handle_t old_obj, esp_gmf_obj_handle_t *new_obj)
{
    ESP_GMF_NULL_CHECK(TAG, old_obj, return ESP_GMF_ERR_INVALID_ARG);
    ESP_GMF_NULL_CHECK(TAG, new_obj, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_obj_t *tmp = (esp_gmf_obj_t *)old_obj;
    return tmp->new_obj(tmp->cfg, new_obj);
}

esp_gmf_err_t esp_gmf_obj_new(esp_gmf_obj_handle_t old_obj, void *cfg, esp_gmf_obj_handle_t *new_obj)
{
    ESP_GMF_NULL_CHECK(TAG, old_obj, return ESP_GMF_ERR_INVALID_ARG);
    ESP_GMF_NULL_CHECK(TAG, cfg, return ESP_GMF_ERR_INVALID_ARG);
    ESP_GMF_NULL_CHECK(TAG, new_obj, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_obj_t *tmp = (esp_gmf_obj_t *)old_obj;
    return tmp->new_obj(cfg, new_obj);
}

esp_gmf_err_t esp_gmf_obj_delete(esp_gmf_obj_handle_t obj)
{
    ESP_GMF_NULL_CHECK(TAG, obj, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_obj_t *h = (esp_gmf_obj_t *)obj;
    ESP_LOGD(TAG, "esp_gmf_obj_delete:%p", obj);
    if (h->tag) {
        esp_gmf_oal_free((void *)h->tag);
        h->tag = NULL;
    }
    return h->del_obj(h);
}

esp_gmf_err_t esp_gmf_obj_set_config(esp_gmf_obj_handle_t obj, void *cfg, int cfg_size)
{
    ESP_GMF_NULL_CHECK(TAG, obj, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_obj_t *h = (esp_gmf_obj_t *)obj;
    h->cfg = cfg;
    return ESP_GMF_ERR_OK;
}

esp_gmf_err_t esp_gmf_obj_set_tag(esp_gmf_obj_handle_t obj, const char *tag)
{
    ESP_GMF_NULL_CHECK(TAG, obj, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_obj_t *h = (esp_gmf_obj_t *)obj;
    if (h->tag) {
        esp_gmf_oal_free((void *)h->tag);
        h->tag = NULL;
    }
    if (tag) {
        if (strlen(tag) >= ESP_GMF_TAG_MAX_LEN) {
            ESP_LOGE(TAG, "The length of tag is out of range, len:%d", strlen(tag));
            return ESP_GMF_ERR_INVALID_ARG;
        }
        h->tag = esp_gmf_oal_strdup(tag);
        ESP_GMF_MEM_CHECK(TAG, h->tag, return ESP_GMF_ERR_MEMORY_LACK);
    }
    return ESP_GMF_ERR_OK;
}

esp_gmf_err_t esp_gmf_obj_get_tag(esp_gmf_obj_handle_t obj, char **tag)
{
    ESP_GMF_NULL_CHECK(TAG, obj, return ESP_GMF_ERR_INVALID_ARG);
    ESP_GMF_NULL_CHECK(TAG, tag, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_obj_t *h = (esp_gmf_obj_t *)obj;
    *tag = (char *)h->tag;
    return ESP_GMF_ERR_OK;
}
