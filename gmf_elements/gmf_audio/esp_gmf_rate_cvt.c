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
#include "esp_gmf_node.h"
#include "esp_gmf_audio_element.h"
#include "esp_gmf_rate_cvt.h"
#include "gmf_audio_common.h"
#include "esp_gmf_audio_method_def.h"
#include "esp_gmf_cap.h"
#include "esp_gmf_caps_def.h"

/**
 * @brief Audio rate conversion context in GMF
 */
typedef struct {
    esp_gmf_audio_element_t  parent;           /*!< The GMF rate cvt handle */
    esp_ae_rate_cvt_handle_t rate_hd;          /*!< The audio effects rate cvt handle */
    uint8_t                  bytes_per_sample; /*!< Bytes number of per sampling point */
} esp_gmf_rate_cvt_t;

static const char *TAG = "ESP_GMF_RATE_CVT";

static inline void rate_cvt_change_src_info(esp_gmf_audio_element_handle_t self, uint32_t src_rate, uint8_t src_ch, uint8_t src_bits)
{
    esp_ae_rate_cvt_cfg_t *rate_cvt_info = (esp_ae_rate_cvt_cfg_t *)OBJ_GET_CFG(self);
    rate_cvt_info->channel = src_ch;
    rate_cvt_info->src_rate = src_rate;
    rate_cvt_info->bits_per_sample = src_bits;
}

static esp_gmf_err_t __rate_cvt_set_dest_rate(esp_gmf_audio_element_handle_t handle, esp_gmf_args_desc_t *arg_desc,
                                              uint8_t *buf, int buf_len)
{
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, arg_desc, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, buf, {return ESP_GMF_ERR_INVALID_ARG;});
    uint32_t dest_rate = *((uint32_t *)buf);
    esp_gmf_event_state_t state = -1;
    esp_gmf_element_get_state(handle, &state);
    if (state < ESP_GMF_EVENT_STATE_OPENING) {
        esp_ae_rate_cvt_cfg_t *rate_info = (esp_ae_rate_cvt_cfg_t *)OBJ_GET_CFG(handle);
        ESP_GMF_NULL_CHECK(TAG, rate_info, {return ESP_GMF_ERR_FAIL;});
        rate_info->dest_rate = dest_rate;
    } else {
        ESP_LOGE(TAG, "Failed to set destination rate due to invalid state: %s", esp_gmf_event_get_state_str(state));
        return ESP_GMF_ERR_FAIL;
    }
    return ESP_GMF_ERR_OK;
}

static esp_gmf_err_t esp_gmf_rate_cvt_new(void *cfg, esp_gmf_obj_handle_t *handle)
{
    *handle = NULL;
    esp_ae_rate_cvt_cfg_t *rate_cvt_cfg = (esp_ae_rate_cvt_cfg_t *)cfg;
    esp_gmf_obj_handle_t new_obj = NULL;
    esp_gmf_err_t ret = esp_gmf_rate_cvt_init(rate_cvt_cfg, &new_obj);
    if (ret != ESP_GMF_ERR_OK) {
        return ret;
    }
    ret = esp_gmf_rate_cvt_cast(rate_cvt_cfg, new_obj);
    if (ret != ESP_GMF_ERR_OK) {
        esp_gmf_obj_delete(new_obj);
        return ret;
    }
    *handle = (void *)new_obj;
    return ret;
}

static esp_gmf_job_err_t esp_gmf_rate_cvt_open(esp_gmf_audio_element_handle_t self, void *para)
{
    esp_gmf_rate_cvt_t *rate_cvt = (esp_gmf_rate_cvt_t *)self;
    esp_ae_rate_cvt_cfg_t *rate_info = (esp_ae_rate_cvt_cfg_t *)OBJ_GET_CFG(self);
    ESP_GMF_NULL_CHECK(TAG, rate_info, {return ESP_GMF_JOB_ERR_FAIL;});
    rate_cvt->bytes_per_sample = (rate_info->bits_per_sample >> 3) * rate_info->channel;
    esp_ae_rate_cvt_open(rate_info, &rate_cvt->rate_hd);
    ESP_GMF_CHECK(TAG, rate_cvt->rate_hd, {return ESP_GMF_JOB_ERR_FAIL;}, "Failed to create rate conversion handle");
    GMF_AUDIO_UPDATE_SND_INFO(self, rate_info->dest_rate, rate_info->bits_per_sample, rate_info->channel);
    ESP_LOGD(TAG, "Open, src: %"PRIu32", dest: %"PRIu32", ch: %d, bits: %d",
             rate_info->src_rate, rate_info->dest_rate, rate_info->channel, rate_info->bits_per_sample);
    return ESP_GMF_JOB_ERR_OK;
}

static esp_gmf_job_err_t esp_gmf_rate_cvt_process(esp_gmf_audio_element_handle_t self, void *para)
{
    esp_gmf_rate_cvt_t *rate_cvt = (esp_gmf_rate_cvt_t *)self;
    int out_len = -1;
    esp_ae_err_t ret = ESP_AE_ERR_OK;
    esp_gmf_port_handle_t in_port = ESP_GMF_ELEMENT_GET(self)->in;
    esp_gmf_port_handle_t out_port = ESP_GMF_ELEMENT_GET(self)->out;
    esp_gmf_payload_t *in_load = NULL;
    esp_gmf_payload_t *out_load = NULL;
    esp_gmf_err_io_t load_ret = esp_gmf_port_acquire_in(in_port, &in_load, ESP_GMF_ELEMENT_GET(rate_cvt)->in_attr.data_size, ESP_GMF_MAX_DELAY);
    ESP_GMF_PORT_ACQUIRE_IN_CHECK(TAG, load_ret, out_len, {goto __rate_release;});
    int samples_num = in_load->valid_size / rate_cvt->bytes_per_sample;
    uint32_t out_samples_num = 0;
    if (samples_num) {
        ret = esp_ae_rate_cvt_get_max_out_sample_num(rate_cvt->rate_hd, samples_num, &out_samples_num);
        ESP_GMF_RET_ON_ERROR(TAG, ret, {out_len = ESP_GMF_JOB_ERR_FAIL; goto __rate_release;}, "Failed to get resample out size, ret: %d", ret);
    }
    esp_ae_rate_cvt_cfg_t *rate_cvt_info = (esp_ae_rate_cvt_cfg_t *)OBJ_GET_CFG(self);
    int acq_out_size = out_samples_num == 0 ? in_load->buf_length : out_samples_num * rate_cvt->bytes_per_sample;
    if ((rate_cvt_info->src_rate == rate_cvt_info->dest_rate) && (in_port->is_shared == true)) {
        // This case rate conversion is do bypass
        out_load = in_load;
    }
    load_ret = esp_gmf_port_acquire_out(out_port, &out_load, acq_out_size, ESP_GMF_MAX_DELAY);
    ESP_GMF_PORT_ACQUIRE_OUT_CHECK(TAG, load_ret, out_len, {goto __rate_release;});
    if (samples_num) {
        ret = esp_ae_rate_cvt_process(rate_cvt->rate_hd, (unsigned char *)in_load->buf, samples_num,
                                      (unsigned char *)out_load->buf, &out_samples_num);
        ESP_GMF_RET_ON_ERROR(TAG, ret, {out_len = ESP_GMF_JOB_ERR_FAIL; goto __rate_release;}, "Rate conversion process error, ret: %d", ret);
    }
    out_load->valid_size = out_samples_num * rate_cvt->bytes_per_sample;
    out_load->pts = in_load->pts;
    out_load->is_done = in_load->is_done;
    out_len = out_load->valid_size;
    ESP_LOGV(TAG, "Out Samples: %ld, IN-PLD: %p-%p-%d-%d-%d, OUT-PLD: %p-%p-%d-%d-%d", out_samples_num, in_load, in_load->buf,
             in_load->valid_size, in_load->buf_length, in_load->is_done, out_load,
             out_load->buf, out_load->valid_size, out_load->buf_length, out_load->is_done);
    esp_gmf_audio_el_update_file_pos((esp_gmf_element_handle_t)self, out_load->valid_size);
    if (in_load->is_done) {
        out_len = ESP_GMF_JOB_ERR_DONE;
        ESP_LOGD(TAG, "Rate convert done, out len: %d", out_load->valid_size);
    }
__rate_release:
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

static esp_gmf_job_err_t esp_gmf_rate_cvt_close(esp_gmf_audio_element_handle_t self, void *para)
{
    esp_gmf_rate_cvt_t *rate_cvt = (esp_gmf_rate_cvt_t *)self;
    ESP_LOGD(TAG, "Closed, %p", self);
    if (rate_cvt->rate_hd != NULL) {
        esp_ae_rate_cvt_close(rate_cvt->rate_hd);
        rate_cvt->rate_hd = NULL;
    }
    return ESP_GMF_JOB_ERR_OK;
}

static esp_gmf_err_t rate_cvt_received_event_handler(esp_gmf_event_pkt_t *evt, void *ctx)
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
            rate_cvt_change_src_info(self, info.sample_rates, info.channels, info.bits);
            ESP_LOGD(TAG, "RECV info, from: %s-%p, next: %p, self: %s-%p, type: %x, state: %s, rate: %d, ch: %d, bits: %d",
                     OBJ_GET_TAG(el), el, esp_gmf_node_for_next((esp_gmf_node_t *)el), OBJ_GET_TAG(self), self, evt->type,
                     esp_gmf_event_get_state_str(state), info.sample_rates, info.channels, info.bits);
            // Change the state to ESP_GMF_EVENT_STATE_INITIALIZED, then add to working list.
            esp_gmf_element_set_state(self, ESP_GMF_EVENT_STATE_INITIALIZED);
        }
    }
    return ESP_GMF_ERR_OK;
}

static esp_gmf_err_t esp_gmf_rate_cvt_destroy(esp_gmf_audio_element_handle_t self)
{
    esp_gmf_rate_cvt_t *rate_cvt = (esp_gmf_rate_cvt_t *)self;
    ESP_LOGD(TAG, "Destroyed, %p", self);
    void *cfg = OBJ_GET_CFG(self);
    if (cfg) {
        esp_gmf_oal_free(cfg);
    }
    esp_gmf_audio_el_deinit(self);
    esp_gmf_oal_free(rate_cvt);
    return ESP_GMF_ERR_OK;
}

static esp_gmf_err_t _load_rate_cvt_caps_func(esp_gmf_cap_t **caps)
{
    ESP_GMF_MEM_CHECK(TAG, caps, return ESP_ERR_INVALID_ARG);
    esp_gmf_cap_t dec_caps = {0};
    dec_caps.cap_eightcc = ESP_GMF_CAPS_AUDIO_RATE_CONVERT;
    dec_caps.attr_fun = NULL;
    int ret = esp_gmf_cap_append(caps, &dec_caps);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, {return ret;}, "Failed to create capability");
    return ESP_GMF_ERR_OK;
}

esp_gmf_err_t esp_gmf_rate_cvt_set_dest_rate(esp_gmf_audio_element_handle_t handle, uint32_t dest_rate)
{
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_gmf_method_t *method_head = NULL;
    esp_gmf_method_t *method = NULL;
    esp_gmf_element_get_method((esp_gmf_element_handle_t)handle, &method_head);
    esp_gmf_method_found(method_head, ESP_GMF_METHOD_RATE_CVT_SET_DEST_RATE, &method);
    uint8_t buf[4] = {0};
    esp_gmf_args_set_value(method->args_desc, ESP_GMF_METHOD_RATE_CVT_SET_DEST_RATE_ARG_RATE, buf, (uint8_t *)&dest_rate, sizeof(dest_rate));
    return esp_gmf_element_exe_method((esp_gmf_element_handle_t)handle, ESP_GMF_METHOD_RATE_CVT_SET_DEST_RATE, buf, sizeof(buf));
}

esp_gmf_err_t esp_gmf_rate_cvt_init(esp_ae_rate_cvt_cfg_t *config, esp_gmf_obj_handle_t *handle)
{
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    *handle = NULL;
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
    esp_gmf_rate_cvt_t *rate_cvt = esp_gmf_oal_calloc(1, sizeof(esp_gmf_rate_cvt_t));
    ESP_GMF_MEM_VERIFY(TAG, rate_cvt, {return ESP_GMF_ERR_MEMORY_LACK;}, "rate conversion", sizeof(esp_gmf_rate_cvt_t));
    esp_gmf_obj_t *obj = (esp_gmf_obj_t *)rate_cvt;
    obj->new_obj = esp_gmf_rate_cvt_new;
    obj->del_obj = esp_gmf_rate_cvt_destroy;
    if (config) {
        esp_ae_rate_cvt_cfg_t *cfg = esp_gmf_oal_calloc(1, sizeof(*config));
        ESP_GMF_MEM_VERIFY(TAG, cfg, {ret = ESP_GMF_ERR_MEMORY_LACK; goto RATE_CVT_INIT_FAIL;}, "rate conversion configuration", sizeof(*config));
        memcpy(cfg, config, sizeof(*config));
        esp_gmf_obj_set_config(obj, cfg, sizeof(*config));
    }
    ret = esp_gmf_obj_set_tag(obj, "rate_cvt");
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, goto RATE_CVT_INIT_FAIL, "Failed to set obj tag");
    esp_gmf_element_cfg_t el_cfg = {0};
    ESP_GMF_ELEMENT_IN_PORT_ATTR_SET(el_cfg.in_attr, ESP_GMF_EL_PORT_CAP_SINGLE, 0, 0,
        ESP_GMF_PORT_TYPE_BLOCK | ESP_GMF_PORT_TYPE_BYTE, ESP_GMF_ELEMENT_PORT_DATA_SIZE_DEFAULT);
    ESP_GMF_ELEMENT_IN_PORT_ATTR_SET(el_cfg.out_attr, ESP_GMF_EL_PORT_CAP_SINGLE, 0, 0,
        ESP_GMF_PORT_TYPE_BLOCK | ESP_GMF_PORT_TYPE_BYTE, ESP_GMF_ELEMENT_PORT_DATA_SIZE_DEFAULT);
    el_cfg.dependency = true;
    ret = esp_gmf_audio_el_init(rate_cvt, &el_cfg);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, goto RATE_CVT_INIT_FAIL, "Failed to initialize rate conversion element");
    *handle = obj;
    ESP_LOGD(TAG, "Initialization, %s-%p", OBJ_GET_TAG(obj), obj);
    return ESP_GMF_ERR_OK;
RATE_CVT_INIT_FAIL:
    esp_gmf_obj_delete(obj);
    return ret;
}

esp_gmf_err_t esp_gmf_rate_cvt_cast(esp_ae_rate_cvt_cfg_t *config, esp_gmf_obj_handle_t handle)
{
    ESP_GMF_NULL_CHECK(TAG, config, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_ae_rate_cvt_cfg_t *cfg = esp_gmf_oal_calloc(1, sizeof(*config));
    ESP_GMF_MEM_VERIFY(TAG, cfg, {return ESP_GMF_ERR_MEMORY_LACK;}, "rate conversion configuration", sizeof(*config));
    memcpy(cfg, config, sizeof(*config));
    // Free memory before overwriting
    esp_gmf_oal_free(OBJ_GET_CFG(handle));
    esp_gmf_obj_set_config(handle, cfg, sizeof(*config));
    esp_gmf_audio_element_t *rate_cvt_el = (esp_gmf_audio_element_t *)handle;
    esp_gmf_args_desc_t *set_args = NULL;

    esp_gmf_err_t ret = esp_gmf_args_desc_append(&set_args, ESP_GMF_METHOD_RATE_CVT_SET_DEST_RATE_ARG_RATE,
                                                 ESP_GMF_ARGS_TYPE_UINT32, sizeof(uint32_t), 0);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, {return ret;}, "Failed to append argument");
    ret = esp_gmf_element_register_method(rate_cvt_el, ESP_GMF_METHOD_RATE_CVT_SET_DEST_RATE, __rate_cvt_set_dest_rate, set_args);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, {return ret;}, "Failed to register method");

    rate_cvt_el->base.ops.open = esp_gmf_rate_cvt_open;
    rate_cvt_el->base.ops.process = esp_gmf_rate_cvt_process;
    rate_cvt_el->base.ops.close = esp_gmf_rate_cvt_close;
    rate_cvt_el->base.ops.event_receiver = rate_cvt_received_event_handler;
    rate_cvt_el->base.ops.load_caps = _load_rate_cvt_caps_func;
    return ESP_GMF_ERR_OK;
}
