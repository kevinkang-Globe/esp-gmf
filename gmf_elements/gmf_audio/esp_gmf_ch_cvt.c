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
#include "esp_gmf_node.h"
#include "esp_gmf_oal_mem.h"
#include "esp_gmf_audio_element.h"
#include "esp_gmf_ch_cvt.h"
#include "gmf_audio_common.h"
#include "esp_gmf_audio_method_def.h"

/**
 * @brief Audio channel conversion context in GMF
 */
typedef struct {
    esp_gmf_audio_element_t parent;               /*!< The GMF channel cvt handle */
    esp_ae_ch_cvt_handle_t  ch_hd;                /*!< The audio effects channel cvt handle */
    uint8_t                 in_bytes_per_sample;  /*!< Source bytes number of per sampling point */
    uint8_t                 out_bytes_per_sample; /*!< Dest bytes number of per sampling point */
} esp_gmf_ch_cvt_t;

static const char *TAG = "ESP_GMF_CH_CVT";

static inline esp_gmf_err_t dupl_esp_ae_ch_cvt_cfg(esp_ae_ch_cvt_cfg_t *config, esp_ae_ch_cvt_cfg_t **new_config)
{
    void *sub_cfg = NULL;
    *new_config = esp_gmf_oal_calloc(1, sizeof(*config));
    ESP_GMF_MEM_VERIFY(TAG, *new_config, {return ESP_GMF_ERR_MEMORY_LACK;}, "channel conversion configuration", sizeof(*config));
    memcpy(*new_config, config, sizeof(*config));
    if (config->weight && (config->weight_len > 0)) {
        sub_cfg = esp_gmf_oal_calloc(1, config->weight_len);
        ESP_GMF_MEM_VERIFY(TAG, sub_cfg, {esp_gmf_oal_free(*new_config); return ESP_GMF_ERR_MEMORY_LACK;},
                           "weight array", (int)config->weight_len);
        memcpy(sub_cfg, config->weight, config->weight_len);
        (*new_config)->weight = sub_cfg;
    }
    return ESP_GMF_JOB_ERR_OK;
}

static inline void free_esp_ae_ch_cvt_cfg(esp_ae_ch_cvt_cfg_t *config)
{
    if (config && config->weight) {
        esp_gmf_oal_free(config->weight);
        config->weight = NULL;
        config->weight_len = 0;
    }
    esp_gmf_oal_free(config);
}

static inline void ch_cvt_change_src_info(esp_gmf_audio_element_handle_t self, uint32_t src_rate, uint8_t src_ch, uint8_t src_bits)
{
    esp_ae_ch_cvt_cfg_t *ch_info = (esp_ae_ch_cvt_cfg_t *)OBJ_GET_CFG(self);
    ch_info->src_ch = src_ch;
    ch_info->sample_rate = src_rate;
    ch_info->bits_per_sample = src_bits;
}

static esp_gmf_err_t __set_dest_ch(esp_gmf_audio_element_handle_t handle, esp_gmf_args_desc_t *arg_desc,
                                   uint8_t *buf, int buf_len)
{
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, arg_desc, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, buf, {return ESP_GMF_ERR_INVALID_ARG;});
    uint8_t dest_ch = (uint8_t)(*buf);
    esp_gmf_event_state_t state = -1;
    esp_gmf_element_get_state(handle, &state);
    if (state < ESP_GMF_EVENT_STATE_OPENING) {
        esp_ae_ch_cvt_cfg_t *ch_info = (esp_ae_ch_cvt_cfg_t *)OBJ_GET_CFG(handle);
        ESP_GMF_NULL_CHECK(TAG, ch_info, {return ESP_GMF_ERR_FAIL;});
        ch_info->dest_ch = dest_ch;
    } else {
        ESP_LOGE(TAG, "Failed to set destination channel due to invalid state: %s", esp_gmf_event_get_state_str(state));
        return ESP_GMF_ERR_FAIL;
    }
    return ESP_GMF_ERR_OK;
}

static esp_gmf_err_t esp_gmf_ch_cvt_new(void *cfg, esp_gmf_obj_handle_t *handle)
{
    ESP_GMF_NULL_CHECK(TAG, cfg, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    *handle = NULL;
    esp_ae_ch_cvt_cfg_t *ch_cvt_cfg = (esp_ae_ch_cvt_cfg_t *)cfg;
    esp_gmf_obj_handle_t new_obj = NULL;
    esp_gmf_err_t ret = esp_gmf_ch_cvt_init(ch_cvt_cfg, &new_obj);
    if (ret != ESP_GMF_ERR_OK) {
        return ret;
    }
    ret = esp_gmf_ch_cvt_cast(ch_cvt_cfg, new_obj);
    if (ret != ESP_GMF_ERR_OK) {
        esp_gmf_obj_delete(new_obj);
        return ret;
    }
    *handle = (void *)new_obj;
    return ret;
}

static esp_gmf_job_err_t esp_gmf_ch_cvt_open(esp_gmf_audio_element_handle_t self, void *para)
{
    ESP_GMF_NULL_CHECK(TAG, self, {return ESP_GMF_JOB_ERR_FAIL;});
    esp_gmf_ch_cvt_t *ch_cvt = (esp_gmf_ch_cvt_t *)self;
    esp_ae_ch_cvt_cfg_t *ch_info = (esp_ae_ch_cvt_cfg_t *)OBJ_GET_CFG(self);
    ESP_GMF_NULL_CHECK(TAG, ch_info, {return ESP_GMF_JOB_ERR_FAIL;});
    esp_ae_ch_cvt_open(ch_info, &ch_cvt->ch_hd);
    ESP_GMF_CHECK(TAG, ch_cvt->ch_hd, {return ESP_GMF_JOB_ERR_FAIL;}, "Failed to create channel conversion handle");
    ch_cvt->in_bytes_per_sample = (ch_info->bits_per_sample >> 3) * ch_info->src_ch;
    ch_cvt->out_bytes_per_sample = (ch_info->bits_per_sample >> 3) * ch_info->dest_ch;
    GMF_AUDIO_UPDATE_SND_INFO(self, ch_info->sample_rate, ch_info->bits_per_sample, ch_info->dest_ch);
    ESP_LOGD(TAG, "Open, rate: %ld, bits: %d, src_channel: %d, dest_channel: %d",
             ch_info->sample_rate, ch_info->bits_per_sample, ch_info->src_ch, ch_info->dest_ch);
    return ESP_GMF_JOB_ERR_OK;
}

static esp_gmf_job_err_t esp_gmf_ch_cvt_process(esp_gmf_audio_element_handle_t self, void *para)
{
    ESP_GMF_NULL_CHECK(TAG, self, {return ESP_GMF_JOB_ERR_FAIL;});
    esp_gmf_ch_cvt_t *ch_cvt = (esp_gmf_ch_cvt_t *)self;
    int out_len = -1;
    esp_gmf_port_handle_t in_port = ESP_GMF_ELEMENT_GET(self)->in;
    esp_gmf_port_handle_t out_port = ESP_GMF_ELEMENT_GET(self)->out;
    esp_gmf_payload_t *in_load = NULL;
    esp_gmf_payload_t *out_load = NULL;
    esp_gmf_err_io_t load_ret = esp_gmf_port_acquire_in(in_port, &in_load, GMF_AUDIO_INPUT_SAMPLE_NUM * ch_cvt->in_bytes_per_sample, ESP_GMF_MAX_DELAY);
    ESP_GMF_PORT_ACQUIRE_IN_CHECK(TAG, load_ret, out_len, {goto __ch_release;});
    esp_ae_ch_cvt_cfg_t *ch_info = (esp_ae_ch_cvt_cfg_t *)OBJ_GET_CFG(self);
    if ((ch_info->src_ch == ch_info->dest_ch) && (in_port->is_shared == true)) {
        // This case channel conversion is do bypass
        out_load = in_load;
    }
    int samples_num = in_load->valid_size / (ch_cvt->in_bytes_per_sample);
    load_ret = esp_gmf_port_acquire_out(out_port, &out_load, samples_num ? samples_num * ch_cvt->out_bytes_per_sample : in_load->buf_length, ESP_GMF_MAX_DELAY);
    ESP_GMF_PORT_ACQUIRE_OUT_CHECK(TAG, load_ret, out_len, {goto __ch_release;});
    if (samples_num) {
        esp_ae_err_t ret = esp_ae_ch_cvt_process(ch_cvt->ch_hd, samples_num, (unsigned char *)in_load->buf, (unsigned char *)out_load->buf);
        ESP_GMF_RET_ON_ERROR(TAG, ret, {out_len = ESP_GMF_JOB_ERR_FAIL; goto __ch_release;}, "Channel conversion process error, ret: %d", ret);
    }
    out_load->valid_size = samples_num * ch_cvt->out_bytes_per_sample;
    out_load->pts = in_load->pts;
    out_load->is_done = in_load->is_done;
    out_len = out_load->valid_size;
    ESP_LOGV(TAG, "Samples: %d, IN-PLD: %p-%p-%d-%d-%d, OUT-PLD: %p-%p-%d-%d-%d",
             samples_num, in_load, in_load->buf, in_load->valid_size, in_load->buf_length, in_load->is_done,
             out_load, out_load->buf, out_load->valid_size, out_load->buf_length, out_load->is_done);
    if (out_load->valid_size > 0) {
        esp_gmf_audio_el_update_file_pos((esp_gmf_element_handle_t)self, out_load->valid_size);
    }
    if (in_load->is_done) {
        out_len = ESP_GMF_JOB_ERR_DONE;
        ESP_LOGD(TAG, "The channel cvt done, out len: %d", out_load->valid_size);
    }
__ch_release:
    if (in_load != NULL) {
        load_ret = esp_gmf_port_release_in(in_port, in_load, ESP_GMF_MAX_DELAY);
        ESP_GMF_PORT_RELEASE_IN_CHECK(TAG, load_ret, out_len, NULL);
    }
    if (out_load != NULL) {
        load_ret = esp_gmf_port_release_out(out_port, out_load, ESP_GMF_MAX_DELAY);
        ESP_GMF_PORT_RELEASE_OUT_CHECK(TAG, load_ret, out_len, NULL);
    }
    return out_len;
}

static esp_gmf_job_err_t esp_gmf_ch_cvt_close(esp_gmf_audio_element_handle_t self, void *para)
{
    ESP_GMF_NULL_CHECK(TAG, self, {return ESP_GMF_ERR_OK;});
    esp_gmf_ch_cvt_t *ch_cvt = (esp_gmf_ch_cvt_t *)self;
    ESP_LOGD(TAG, "Closed, %p", self);
    if (ch_cvt->ch_hd != NULL) {
        esp_ae_ch_cvt_close(ch_cvt->ch_hd);
        ch_cvt->ch_hd = NULL;
    }
    return ESP_GMF_JOB_ERR_OK;
}

static esp_gmf_err_t ch_cvt_received_event_handler(esp_gmf_event_pkt_t *evt, void *ctx)
{
    ESP_GMF_NULL_CHECK(TAG, evt, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, ctx, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_gmf_element_handle_t self = (esp_gmf_element_handle_t)ctx;
    esp_gmf_element_handle_t el = evt->from;
    esp_gmf_event_state_t state = ESP_GMF_EVENT_STATE_NONE;
    esp_gmf_element_get_state(self, &state);
    esp_gmf_element_handle_t prev = NULL;
    esp_gmf_element_get_prev_el(self, &prev);
    if ((state == ESP_GMF_EVENT_STATE_NONE) || (prev == el)) {
        if (evt->sub == ESP_GMF_INFO_SOUND) {
            esp_gmf_info_sound_t info = {0};
            memcpy(&info, evt->payload, evt->payload_size);
            ch_cvt_change_src_info(self, info.sample_rates, info.channels, info.bits);
            ESP_LOGD(TAG, "RECV info, from: %s-%p, next: %p, self: %s-%p, type: %x, state: %s, rate: %d, ch: %d, bits: %d",
                     OBJ_GET_TAG(el), el, esp_gmf_node_for_next((esp_gmf_node_t *)el), OBJ_GET_TAG(self), self, evt->type,
                     esp_gmf_event_get_state_str(state), info.sample_rates, info.channels, info.bits);
            // Change the state to ESP_GMF_EVENT_STATE_INITIALIZED, then add to working list.
            esp_gmf_element_set_state(self, ESP_GMF_EVENT_STATE_INITIALIZED);
        }
    }
    return ESP_GMF_ERR_OK;
}

static esp_gmf_err_t esp_gmf_ch_cvt_destroy(esp_gmf_audio_element_handle_t self)
{
    if (self != NULL) {
        esp_gmf_ch_cvt_t *ch_cvt = (esp_gmf_ch_cvt_t *)self;
        ESP_LOGD(TAG, "Destroyed, %p", self);
        free_esp_ae_ch_cvt_cfg(OBJ_GET_CFG(self));
        esp_gmf_audio_el_deinit(self);
        esp_gmf_oal_free(ch_cvt);
    }
    return ESP_GMF_ERR_OK;
}

esp_gmf_err_t esp_gmf_ch_cvt_set_dest_channel(esp_gmf_audio_element_handle_t handle, uint8_t dest_ch)
{
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_gmf_method_t *method_head = NULL;
    esp_gmf_method_t *method = NULL;
    esp_gmf_element_get_method((esp_gmf_element_handle_t)handle, &method_head);
    esp_gmf_method_found(method_head, ESP_GMF_METHOD_CH_CVT_SET_DEST_CH, &method);
    uint8_t buf[1] = {0};
    esp_gmf_args_set_value(method->args_desc, ESP_GMF_METHOD_CH_CVT_SET_DEST_CH_ARG_CH, buf, (uint8_t *)&dest_ch, sizeof(dest_ch));
    return esp_gmf_element_exe_method((esp_gmf_element_handle_t)handle, ESP_GMF_METHOD_CH_CVT_SET_DEST_CH, buf, sizeof(buf));
}

esp_gmf_err_t esp_gmf_ch_cvt_init(esp_ae_ch_cvt_cfg_t *config, esp_gmf_obj_handle_t *handle)
{
    ESP_GMF_NULL_CHECK(TAG, config, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    *handle = NULL;
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
    esp_gmf_ch_cvt_t *ch_cvt = esp_gmf_oal_calloc(1, sizeof(esp_gmf_ch_cvt_t));
    ESP_GMF_MEM_VERIFY(TAG, ch_cvt, {return ESP_GMF_ERR_MEMORY_LACK;}, "channel conversion", sizeof(esp_gmf_ch_cvt_t));
    esp_gmf_obj_t *obj = (esp_gmf_obj_t *)ch_cvt;
    obj->new_obj = esp_gmf_ch_cvt_new;
    obj->del_obj = esp_gmf_ch_cvt_destroy;
    esp_ae_ch_cvt_cfg_t *new_cfg = NULL;
    dupl_esp_ae_ch_cvt_cfg(config, &new_cfg);
    ESP_GMF_CHECK(TAG, new_cfg, {ret = ESP_GMF_ERR_MEMORY_LACK; goto CH_CVT_INIT_FAIL;}, "Failed to allocate channel conversion configuration");
    esp_gmf_obj_set_config(obj, new_cfg, sizeof(*new_cfg));
    ret = esp_gmf_obj_set_tag(obj, "ch_cvt");
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, goto CH_CVT_INIT_FAIL, "Failed to set obj tag");
    esp_gmf_element_cfg_t el_cfg = {0};
    ESP_GMF_ELEMENT_CFG(el_cfg, true, ESP_GMF_EL_PORT_CAP_SINGLE, ESP_GMF_EL_PORT_CAP_SINGLE,
                        ESP_GMF_PORT_TYPE_BLOCK | ESP_GMF_PORT_TYPE_BYTE, ESP_GMF_PORT_TYPE_BYTE | ESP_GMF_PORT_TYPE_BLOCK);
    ret = esp_gmf_audio_el_init(ch_cvt, &el_cfg);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, goto CH_CVT_INIT_FAIL, "Failed to initialize channel conversion element");
    *handle = obj;
    ESP_LOGD(TAG, "Initialization, %s-%p", OBJ_GET_TAG(obj), obj);
    return ESP_GMF_ERR_OK;
CH_CVT_INIT_FAIL:
    esp_gmf_obj_delete(obj);
    return ret;
}

esp_gmf_err_t esp_gmf_ch_cvt_cast(esp_ae_ch_cvt_cfg_t *config, esp_gmf_obj_handle_t handle)
{
    ESP_GMF_NULL_CHECK(TAG, config, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
    esp_ae_ch_cvt_cfg_t *new_cfg = NULL;
    dupl_esp_ae_ch_cvt_cfg(config, &new_cfg);
    ESP_GMF_CHECK(TAG, new_cfg, {return ESP_GMF_ERR_MEMORY_LACK;}, "Failed to duplicate channel conversion configuration");
    // Free memory before overwriting
    free_esp_ae_ch_cvt_cfg(OBJ_GET_CFG(handle));
    esp_gmf_obj_set_config(handle, new_cfg, sizeof(*config));
    esp_gmf_audio_element_t *ch_cvt_el = (esp_gmf_audio_element_t *)handle;
    esp_gmf_args_desc_t *set_args = NULL;

    ret = esp_gmf_args_desc_append(&set_args, ESP_GMF_METHOD_CH_CVT_SET_DEST_CH_ARG_CH, ESP_GMF_ARGS_TYPE_UINT8, sizeof(uint8_t), 0);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, {return ret;}, "Failed to append argument");
    ret = esp_gmf_element_register_method(ch_cvt_el, ESP_GMF_METHOD_CH_CVT_SET_DEST_CH, __set_dest_ch, set_args);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, {return ret;}, "Failed to register method");

    ch_cvt_el->base.ops.open = esp_gmf_ch_cvt_open;
    ch_cvt_el->base.ops.process = esp_gmf_ch_cvt_process;
    ch_cvt_el->base.ops.close = esp_gmf_ch_cvt_close;
    ch_cvt_el->base.ops.event_receiver = ch_cvt_received_event_handler;
    return ESP_GMF_ERR_OK;
}
