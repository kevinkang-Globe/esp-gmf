/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2025 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD.>
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

#include <string.h>
#include "esp_log.h"
#include "esp_gmf_oal_mem.h"
#include "esp_gmf_audio_element.h"
#include "esp_gmf_node.h"
#include "esp_gmf_deinterleave.h"
#include "esp_ae_data_weaver.h"
#include "esp_heap_caps.h"
#include "gmf_audio_common.h"

#define ESP_GMF_PROCESS_SAMPLE (256)

/**
 * @brief Audio deinterleave context in GMF
 */
typedef struct {
    esp_gmf_audio_element_t parent;           /*!< The GMF deinterleave handle */
    uint8_t                 bytes_per_sample; /*!< Bytes number of per sampling point */
    esp_gmf_payload_t      *in_load;          /*!< The input payload */
    esp_gmf_payload_t     **out_load;         /*!< The array of output payload */
    uint8_t               **out_arr;          /*!< The array of output buffer pointer */
    uint8_t                 channel;          /*!< The audio channel */
    uint8_t                 bits_per_sample;  /*!< Bits number of per sampling point */
} esp_gmf_deinterleave_t;

static const char *TAG = "ESP_GMF_DEINTLV";

static inline esp_gmf_err_t dupl_esp_ae_deinterleave_cfg(esp_gmf_deinterleave_cfg *config, esp_gmf_deinterleave_cfg **new_config)
{
    *new_config = esp_gmf_oal_calloc(1, sizeof(*config));
    ESP_GMF_MEM_VERIFY(TAG, *new_config, {return ESP_GMF_ERR_MEMORY_LACK;}, "deinterleave configuration", sizeof(*config));
    memcpy(*new_config, config, sizeof(*config));
    return ESP_GMF_JOB_ERR_OK;
}

static inline void free_esp_ae_deinterleave_cfg(esp_gmf_deinterleave_cfg *config)
{
    if (config) {
        esp_gmf_oal_free(config);
    }
}

static inline void deinterleave_change_src_info(esp_gmf_audio_element_handle_t self, uint32_t src_rate, uint8_t src_ch, uint8_t src_bits)
{
    esp_gmf_deinterleave_cfg *deinterleave_info = (esp_gmf_deinterleave_cfg *)OBJ_GET_CFG(self);
    deinterleave_info->sample_rate = src_rate;
    deinterleave_info->channel = src_ch;
    deinterleave_info->bits_per_sample = src_bits;
}

static esp_gmf_err_t esp_gmf_deinterleave_new(void *cfg, esp_gmf_obj_handle_t *handle)
{
    ESP_GMF_NULL_CHECK(TAG, cfg, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    *handle = NULL;
    esp_gmf_deinterleave_cfg *deinterleave_cfg = (esp_gmf_deinterleave_cfg *)cfg;
    esp_gmf_obj_handle_t new_obj = NULL;
    esp_gmf_err_t ret = esp_gmf_deinterleave_init(deinterleave_cfg, &new_obj);
    if (ret != ESP_GMF_ERR_OK) {
        return ret;
    }
    ret = esp_gmf_deinterleave_cast(deinterleave_cfg, new_obj);
    if (ret != ESP_GMF_ERR_OK) {
        esp_gmf_obj_delete(new_obj);
        return ret;
    }
    *handle = (void *)new_obj;
    return ret;
}

static esp_gmf_job_err_t esp_gmf_deinterleave_open(esp_gmf_audio_element_handle_t self, void *para)
{
    ESP_GMF_NULL_CHECK(TAG, self, {return ESP_GMF_JOB_ERR_FAIL;});
    esp_gmf_deinterleave_t *deinterleave = (esp_gmf_deinterleave_t *)self;
    esp_gmf_deinterleave_cfg *deinterleave_info = (esp_gmf_deinterleave_cfg *)OBJ_GET_CFG(self);
    ESP_GMF_NULL_CHECK(TAG, deinterleave_info, {return ESP_GMF_JOB_ERR_FAIL;})
    deinterleave->bytes_per_sample = (deinterleave_info->bits_per_sample >> 3);
    deinterleave->out_load = esp_gmf_oal_calloc(1, sizeof(esp_gmf_payload_t *) * deinterleave_info->channel);
    ESP_GMF_MEM_VERIFY(TAG, deinterleave->out_load, {return ESP_GMF_JOB_ERR_FAIL;},
                       "out load", sizeof(esp_gmf_payload_t *) * deinterleave_info->channel);
    deinterleave->out_arr = esp_gmf_oal_calloc(1, sizeof(uint8_t *) * deinterleave_info->channel);
    ESP_GMF_MEM_VERIFY(TAG, deinterleave->out_arr, {return ESP_GMF_JOB_ERR_FAIL;},
                       "out buffer array", sizeof(uint8_t *) * deinterleave_info->channel);
    GMF_AUDIO_UPDATE_SND_INFO(self, deinterleave_info->sample_rate, deinterleave_info->bits_per_sample, 1);
    deinterleave->channel = deinterleave_info->channel;
    deinterleave->bits_per_sample = deinterleave_info->bits_per_sample;
    ESP_LOGD(TAG, "Open, %p", self);
    return ESP_GMF_ERR_OK;
}

static esp_gmf_job_err_t esp_gmf_deinterleave_process(esp_gmf_audio_element_handle_t self, void *para)
{
    ESP_GMF_NULL_CHECK(TAG, self, {return ESP_GMF_JOB_ERR_FAIL;});
    esp_gmf_deinterleave_t *deinterleave = (esp_gmf_deinterleave_t *)self;
    int out_len = -1;
    int i = 0;
    esp_gmf_port_handle_t in_port = ESP_GMF_ELEMENT_GET(self)->in;
    esp_gmf_port_handle_t out = ESP_GMF_ELEMENT_GET(self)->out;
    esp_gmf_port_handle_t out_port = out;
    esp_gmf_deinterleave_cfg *deinterleave_info = (esp_gmf_deinterleave_cfg *)OBJ_GET_CFG(self);
    deinterleave->in_load = NULL;
    memset(deinterleave->out_load, 0, sizeof(esp_gmf_payload_t *) * deinterleave_info->channel);
    esp_gmf_err_io_t load_ret = esp_gmf_port_acquire_in(in_port, &deinterleave->in_load, ESP_GMF_ELEMENT_GET(deinterleave)->in_attr.data_size, ESP_GMF_MAX_DELAY);
    ESP_GMF_PORT_ACQUIRE_IN_CHECK(TAG, load_ret, out_len, {goto __deintlv_release;});
    ESP_LOGV(TAG, "IN: load: %p, buf: %p, valid size: %d, buf length: %d, done: %d",
             deinterleave->in_load, deinterleave->in_load->buf, deinterleave->in_load->valid_size,
             deinterleave->in_load->buf_length, deinterleave->in_load->is_done);
    int samples_num = deinterleave->in_load->valid_size / (deinterleave->bytes_per_sample * deinterleave_info->channel);
    // Not do deinterleave anymore if one channel failed
    while (out_port != NULL) {
        esp_gmf_err_io_t ret = esp_gmf_port_acquire_out(out_port, &(deinterleave->out_load[i]),
                                                       samples_num ? samples_num * deinterleave->bytes_per_sample : deinterleave->in_load->buf_length, ESP_GMF_MAX_DELAY);
        ESP_GMF_PORT_CHECK(TAG, ret, out_len, {out_len = ESP_GMF_JOB_ERR_FAIL; goto __deintlv_release;}, "Failed to acquire out, idx:%d, ret: %d", i, ret);
        deinterleave->out_arr[i] = deinterleave->out_load[i]->buf;
        out_port = out_port->next;
        i++;
    }
    if (samples_num > 0) {
        esp_ae_err_t proc_ret = esp_ae_deintlv_process(deinterleave->channel, deinterleave->bits_per_sample, samples_num,
                                                  deinterleave->in_load->buf, (void **)deinterleave->out_arr);
        ESP_GMF_RET_ON_ERROR(TAG, proc_ret, {out_len = ESP_GMF_JOB_ERR_FAIL; goto __deintlv_release;}, "Deinterleave process error, ret: %d", proc_ret);
    }
    out_port = out;
    i = 0;
    while (out_port != NULL) {
        deinterleave->out_load[i]->pts = deinterleave->in_load->pts;
        deinterleave->out_load[i]->is_done = deinterleave->in_load->is_done;
        deinterleave->out_load[i]->valid_size = samples_num * deinterleave->bytes_per_sample;
        ESP_LOGV(TAG, "OUT: idx: %d load: %p, buf: %p, valid size: %d, buf length: %d, done: %d",
                 i, deinterleave->out_load[i], deinterleave->out_load[i]->buf, deinterleave->out_load[i]->valid_size,
                 deinterleave->out_load[i]->buf_length, deinterleave->out_load[i]->is_done);
        out_len = deinterleave->out_load[i]->valid_size;
        out_port = out_port->next;
        i++;
    }
    if (deinterleave->in_load->is_done) {
        ESP_LOGD(TAG, "Deinterleave is done");
        out_len = ESP_GMF_JOB_ERR_DONE;
    }
__deintlv_release:
    if (deinterleave->in_load != NULL) {
        load_ret = esp_gmf_port_release_in(in_port, deinterleave->in_load, ESP_GMF_MAX_DELAY);
        ESP_GMF_PORT_RELEASE_IN_CHECK(TAG, load_ret, out_len, NULL);
    }
    out_port = out;
    i = 0;
    while (out_port != NULL && deinterleave->out_load[i] != NULL) {
        load_ret = esp_gmf_port_release_out(out_port, deinterleave->out_load[i], out_port->wait_ticks);
        ESP_GMF_PORT_RELEASE_OUT_CHECK(TAG, load_ret, out_len, NULL);
        out_port = out_port->next;
        i++;
    }
    return out_len;
}

static esp_gmf_job_err_t esp_gmf_deinterleave_close(esp_gmf_audio_element_handle_t self, void *para)
{
    ESP_GMF_NULL_CHECK(TAG, self, {return ESP_GMF_ERR_OK;});
    esp_gmf_deinterleave_t *deinterleave = (esp_gmf_deinterleave_t *)self;
    ESP_LOGD(TAG, "Closed, %p", self);
    if (deinterleave->out_arr != NULL) {
        esp_gmf_oal_free(deinterleave->out_arr);
        deinterleave->out_arr = NULL;
    }
    if (deinterleave->out_load != NULL) {
        esp_gmf_oal_free(deinterleave->out_load);
        deinterleave->out_load = NULL;
    }
    return ESP_GMF_ERR_OK;
}

static esp_gmf_err_t deinterleave_received_event_handler(esp_gmf_event_pkt_t *evt, void *ctx)
{
    ESP_GMF_NULL_CHECK(TAG, evt, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, ctx, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_gmf_element_handle_t self = (esp_gmf_element_handle_t)ctx;
    esp_gmf_element_handle_t el = evt->from;
    esp_gmf_event_state_t state = ESP_GMF_EVENT_STATE_NONE;
    esp_gmf_element_get_state(self, &state);
    esp_gmf_element_handle_t prev = NULL;
    esp_gmf_element_get_prev_el(self, &prev);
    if ((state == ESP_GMF_EVENT_STATE_NONE) && (prev == el)) {
        if (evt->sub == ESP_GMF_INFO_SOUND) {
            esp_gmf_info_sound_t info = {0};
            memcpy(&info, evt->payload, evt->payload_size);
            deinterleave_change_src_info(self, info.sample_rates, info.channels, info.bits);
            ESP_LOGD(TAG, "RECV element info, from: %s-%p, next: %p, self: %s-%p, type: %x, state: %s, rate: %d, ch: %d, bits: %d",
                     OBJ_GET_TAG(el), el, esp_gmf_node_for_next((esp_gmf_node_t *)el), OBJ_GET_TAG(self), self, evt->type,
                     esp_gmf_event_get_state_str(state), info.sample_rates, info.channels, info.bits);
            // Change the state to ESP_GMF_EVENT_STATE_INITIALIZED, then add to working list.
            esp_gmf_element_set_state(self, ESP_GMF_EVENT_STATE_INITIALIZED);
        }
    }
    return ESP_GMF_ERR_OK;
}

static esp_gmf_err_t esp_gmf_deinterleave_destroy(esp_gmf_audio_element_handle_t self)
{
    if (self != NULL) {
        esp_gmf_deinterleave_t *deinterleave = (esp_gmf_deinterleave_t *)self;
        ESP_LOGD(TAG, "Destroyed, %p", self);
        free_esp_ae_deinterleave_cfg(OBJ_GET_CFG(self));
        esp_gmf_audio_el_deinit(self);
        esp_gmf_oal_free(deinterleave);
    }
    return ESP_GMF_ERR_OK;
}

esp_gmf_err_t esp_gmf_deinterleave_init(esp_gmf_deinterleave_cfg *config, esp_gmf_obj_handle_t *handle)
{
    ESP_GMF_NULL_CHECK(TAG, config, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    *handle = NULL;
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
    esp_gmf_deinterleave_t *deinterleave = esp_gmf_oal_calloc(1, sizeof(esp_gmf_deinterleave_t));
    ESP_GMF_MEM_VERIFY(TAG, deinterleave, {return ESP_GMF_ERR_MEMORY_LACK;}, "deinterleave", sizeof(esp_gmf_deinterleave_t));
    esp_gmf_obj_t *obj = (esp_gmf_obj_t *)deinterleave;
    obj->new_obj = esp_gmf_deinterleave_new;
    obj->del_obj = esp_gmf_deinterleave_destroy;
    esp_gmf_deinterleave_cfg *new_config = NULL;
    dupl_esp_ae_deinterleave_cfg(config, &new_config);
    ESP_GMF_CHECK(TAG, new_config, {ret = ESP_GMF_ERR_MEMORY_LACK; goto DEINTLV_INIT_FAIL;}, "Failed to allocate deinterleave configuration");
    esp_gmf_obj_set_config(obj, new_config, sizeof(esp_gmf_deinterleave_cfg));
    ret = esp_gmf_obj_set_tag(obj, "deinterleave");
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, goto DEINTLV_INIT_FAIL, "Failed to set obj tag");
    esp_gmf_element_cfg_t el_cfg = {0};
    ESP_GMF_ELEMENT_IN_PORT_ATTR_SET(el_cfg.in_attr, ESP_GMF_EL_PORT_CAP_SINGLE, 0, 0,
        ESP_GMF_PORT_TYPE_BLOCK | ESP_GMF_PORT_TYPE_BYTE, ESP_GMF_ELEMENT_PORT_DATA_SIZE_DEFAULT);
    ESP_GMF_ELEMENT_IN_PORT_ATTR_SET(el_cfg.out_attr, ESP_GMF_EL_PORT_CAP_MULTI, 0, 0,
        ESP_GMF_PORT_TYPE_BLOCK | ESP_GMF_PORT_TYPE_BYTE, ESP_GMF_ELEMENT_PORT_DATA_SIZE_DEFAULT);
    el_cfg.dependency = true;
    ret = esp_gmf_audio_el_init(deinterleave, &el_cfg);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, goto DEINTLV_INIT_FAIL, "Failed to initialize deinterleave element");
    *handle = obj;
    ESP_LOGD(TAG, "Initialization, %s-%p", OBJ_GET_TAG(obj), obj);
    return ESP_GMF_ERR_OK;
DEINTLV_INIT_FAIL:
    esp_gmf_obj_delete(obj);
    return ret;
}

esp_gmf_err_t esp_gmf_deinterleave_cast(esp_gmf_deinterleave_cfg *config, esp_gmf_obj_handle_t handle)
{
    ESP_GMF_NULL_CHECK(TAG, config, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_gmf_deinterleave_cfg *cfg = NULL;
    dupl_esp_ae_deinterleave_cfg(config, &cfg);
    ESP_GMF_CHECK(TAG, cfg, {return ESP_GMF_ERR_MEMORY_LACK;}, "Failed to duplicate deinterleave configuration");
    // Free memory before overwriting
    free_esp_ae_deinterleave_cfg(OBJ_GET_CFG(handle));
    esp_gmf_obj_set_config(handle, cfg, sizeof(*config));
    esp_gmf_audio_element_t *deinterleave_el = (esp_gmf_audio_element_t *)handle;
    deinterleave_el->base.ops.open = esp_gmf_deinterleave_open;
    deinterleave_el->base.ops.process = esp_gmf_deinterleave_process;
    deinterleave_el->base.ops.close = esp_gmf_deinterleave_close;
    deinterleave_el->base.ops.event_receiver = deinterleave_received_event_handler;
    return ESP_GMF_ERR_OK;
}
