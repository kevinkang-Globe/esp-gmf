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
#include "esp_gmf_alc.h"
#include "esp_gmf_args_desc.h"
#include "gmf_audio_common.h"
#include "esp_gmf_audio_method_def.h"

/**
 * @brief  Information temporarily stored after the user calls the alc set interface
 */
typedef struct {
    bool   is_changed; /*!< The flag of whether the alc gain with a certain channel has been modified */
    int8_t gain;       /*!< The modified gain value of a certain channel number */
} esp_gmf_alc_set_info_t;

/**
 * @brief Audio ALC context in GMF
 */
typedef struct {
    esp_gmf_audio_element_t parent;           /*!< The GMF alc handle */
    esp_ae_alc_handle_t     alc_hd;           /*!< The audio effects alc handle */
    uint8_t                 bytes_per_sample; /*!< Bytes number of per sampling point */
    uint8_t                 channel;          /*!< Audio channel */
    esp_gmf_alc_set_info_t *info;             /*!< Changed information of alc */
} esp_gmf_alc_t;

static const char *TAG = "ESP_GMF_ALC";

static inline esp_gmf_job_err_t alc_update_gain(esp_gmf_alc_t *alc)
{
    esp_ae_err_t ret = ESP_AE_ERR_OK;
    for (int32_t i = 0; i < alc->channel; i++) {
        if (alc->info[i].is_changed == true) {
            ret = esp_ae_alc_set_gain(alc->alc_hd, i, alc->info[i].gain);
            if (ret != ESP_AE_ERR_OK) {
                return ESP_GMF_JOB_ERR_FAIL;
            }
            alc->info[i].is_changed = false;
        }
    }
    return ESP_GMF_JOB_ERR_OK;
}

static esp_gmf_err_t __alc_set_gain(esp_gmf_audio_element_handle_t handle, esp_gmf_args_desc_t *arg_desc,
                                    uint8_t *buf, int buf_len)
{
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, arg_desc, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, buf, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_gmf_alc_t *alc = (esp_gmf_alc_t *)handle;
    ESP_GMF_NULL_CHECK(TAG, alc->info, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_gmf_args_desc_t *alc_desc = arg_desc;
    uint8_t idx = (uint8_t)(*buf);
    alc_desc = alc_desc->next;
    int8_t gain = (int8_t)(*(buf + alc_desc->offset));
    alc->info[idx].is_changed = true;
    alc->info[idx].gain = gain;
    return ESP_GMF_ERR_OK;
}

static esp_gmf_err_t __alc_get_gain(esp_gmf_audio_element_handle_t handle, esp_gmf_args_desc_t *arg_desc,
                                    uint8_t *buf, int buf_len)
{
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, arg_desc, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, buf, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_gmf_alc_t *alc = (esp_gmf_alc_t *)handle;
    esp_gmf_args_desc_t *alc_desc = arg_desc;
    uint8_t idx = (uint8_t)(*buf);
    alc_desc = alc_desc->next;
    int8_t *gain = (int8_t *)(buf + alc_desc->offset);
    esp_ae_err_t ret = esp_ae_alc_get_gain(alc->alc_hd, idx, gain);
    if (ret != ESP_AE_ERR_OK) {
        return ESP_GMF_ERR_FAIL;
    }
    return ESP_GMF_ERR_OK;
}

static inline void alc_change_src_info(esp_gmf_audio_element_handle_t self, uint32_t src_rate, uint8_t src_ch, uint8_t src_bits)
{
    esp_ae_alc_cfg_t *alc_info = (esp_ae_alc_cfg_t *)OBJ_GET_CFG(self);
    alc_info->channel = src_ch;
    alc_info->sample_rate = src_rate;
    alc_info->bits_per_sample = src_bits;
}

static esp_gmf_err_t esp_gmf_alc_new(void *cfg, esp_gmf_obj_handle_t *handle)
{
    ESP_GMF_NULL_CHECK(TAG, cfg, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    *handle = NULL;
    esp_ae_alc_cfg_t *alc_cfg = (esp_ae_alc_cfg_t *)cfg;
    esp_gmf_obj_handle_t new_obj = NULL;
    esp_gmf_err_t ret = esp_gmf_alc_init(alc_cfg, &new_obj);
    if (ret != ESP_GMF_ERR_OK) {
        return ret;
    }
    ret = esp_gmf_alc_cast(alc_cfg, new_obj);
    if (ret != ESP_GMF_ERR_OK) {
        esp_gmf_obj_delete(new_obj);
        return ret;
    }
    *handle = (void *)new_obj;
    return ret;
}

static esp_gmf_job_err_t esp_gmf_alc_open(esp_gmf_audio_element_handle_t self, void *para)
{
    ESP_GMF_NULL_CHECK(TAG, self, {return ESP_GMF_JOB_ERR_FAIL;});
    esp_gmf_alc_t *alc = (esp_gmf_alc_t *)self;
    esp_ae_alc_cfg_t *alc_info = (esp_ae_alc_cfg_t *)OBJ_GET_CFG(self);
    ESP_GMF_NULL_CHECK(TAG, alc_info, {return ESP_GMF_JOB_ERR_FAIL;});
    alc->bytes_per_sample = (alc_info->bits_per_sample >> 3) * alc_info->channel;
    esp_ae_alc_open(alc_info, &alc->alc_hd);
    ESP_GMF_CHECK(TAG, alc->alc_hd, {return ESP_GMF_JOB_ERR_FAIL;}, "Failed to create alc handle");
    GMF_AUDIO_UPDATE_SND_INFO(self, alc_info->sample_rate, alc_info->bits_per_sample, alc_info->channel);
    alc->channel = alc_info->channel;
    alc->info = esp_gmf_oal_calloc(1, alc_info->channel * sizeof(esp_gmf_alc_set_info_t));
    ESP_GMF_MEM_VERIFY(TAG, alc->info, {return ESP_GMF_JOB_ERR_FAIL;}, "alc information", alc_info->channel * sizeof(esp_gmf_alc_set_info_t));
    ESP_LOGD(TAG, "Open, %p", self);
    return ESP_GMF_JOB_ERR_OK;
}

static esp_gmf_job_err_t esp_gmf_alc_process(esp_gmf_audio_element_handle_t self, void *para)
{
    ESP_GMF_NULL_CHECK(TAG, self, {return ESP_GMF_JOB_ERR_FAIL;});
    esp_gmf_alc_t *alc = (esp_gmf_alc_t *)self;
    int out_len = -1;
    esp_gmf_port_handle_t in_port = ESP_GMF_ELEMENT_GET(self)->in;
    esp_gmf_port_handle_t out_port = ESP_GMF_ELEMENT_GET(self)->out;
    esp_gmf_payload_t *in_load = NULL;
    esp_gmf_payload_t *out_load = NULL;
    // parameter set
    ESP_GMF_RET_ON_NOT_OK(TAG, alc_update_gain(alc), {return ESP_GMF_JOB_ERR_FAIL;}, "Failed to update gain");
    esp_gmf_err_io_t load_ret = esp_gmf_port_acquire_in(in_port, &in_load, ESP_GMF_ELEMENT_GET(alc)->in_attr.data_size, ESP_GMF_MAX_DELAY);
    ESP_GMF_PORT_ACQUIRE_IN_CHECK(TAG, load_ret, out_len, {goto __alc_release;});
    int samples_num = in_load->valid_size / (alc->bytes_per_sample);
    if (in_port->is_shared == 1) {
        out_load = in_load;
    }
    load_ret = esp_gmf_port_acquire_out(out_port, &out_load, samples_num ? samples_num * alc->bytes_per_sample : in_load->buf_length, ESP_GMF_MAX_DELAY);
    ESP_GMF_PORT_ACQUIRE_OUT_CHECK(TAG, load_ret, out_len, {goto __alc_release;});
    if (samples_num > 0) {
        esp_ae_err_t ret = esp_ae_alc_process(alc->alc_hd, samples_num, in_load->buf, out_load->buf);
        ESP_GMF_RET_ON_ERROR(TAG, ret, {out_len = ESP_GMF_JOB_ERR_FAIL; goto __alc_release;}, "ALC process error %d", ret);
    }
    ESP_LOGV(TAG, "Samples: %d, IN-PLD: %p-%p-%d-%d-%d, OUT-PLD: %p-%p-%d-%d-%d",
             samples_num, in_load, in_load->buf, in_load->valid_size, in_load->buf_length, in_load->is_done,
             out_load, out_load->buf, out_load->valid_size, out_load->buf_length, out_load->is_done);
    out_load->valid_size = samples_num * alc->bytes_per_sample;
    out_load->is_done = in_load->is_done;
    out_len = out_load->valid_size;
    esp_gmf_audio_el_update_file_pos((esp_gmf_element_handle_t)self, out_len);
    if (in_load->is_done) {
        out_len = ESP_GMF_JOB_ERR_DONE;
        ESP_LOGD(TAG, "ALC done, out len: %d", out_load->valid_size);
    }
__alc_release:
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

static esp_gmf_job_err_t esp_gmf_alc_close(esp_gmf_audio_element_handle_t self, void *para)
{
    ESP_GMF_NULL_CHECK(TAG, self, {return ESP_GMF_ERR_OK;});
    esp_gmf_alc_t *alc = (esp_gmf_alc_t *)self;
    ESP_LOGD(TAG, "Closed, %p", self);
    if (alc->alc_hd != NULL) {
        esp_ae_alc_close(alc->alc_hd);
        alc->alc_hd = NULL;
    }
    if (alc->info != NULL) {
        esp_gmf_oal_free(alc->info);
        alc->info = NULL;
    }
    return ESP_GMF_ERR_OK;
}

static esp_gmf_err_t alc_received_event_handler(esp_gmf_event_pkt_t *evt, void *ctx)
{
    ESP_GMF_NULL_CHECK(TAG, ctx, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, evt, {return ESP_GMF_ERR_INVALID_ARG;});
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
            alc_change_src_info(self, info.sample_rates, info.channels, info.bits);
            ESP_LOGD(TAG, "RECV element info, from: %s-%p, next: %p, self: %s-%p, type: %x, state: %s, rate: %d, ch: %d, bits: %d",
                     OBJ_GET_TAG(el), el, esp_gmf_node_for_next((esp_gmf_node_t *)el), OBJ_GET_TAG(self), self, evt->type,
                     esp_gmf_event_get_state_str(state), info.sample_rates, info.channels, info.bits);
            // Change the state to ESP_GMF_EVENT_STATE_INITIALIZED, then add to working list.
            esp_gmf_element_set_state(self, ESP_GMF_EVENT_STATE_INITIALIZED);
        }
    }
    return ESP_GMF_ERR_OK;
}

static esp_gmf_err_t esp_gmf_alc_destroy(esp_gmf_audio_element_handle_t self)
{
    if (self != NULL) {
        esp_gmf_alc_t *alc = (esp_gmf_alc_t *)self;
        ESP_LOGD(TAG, "Destroyed, %p", self);
        esp_gmf_oal_free(OBJ_GET_CFG(self));
        esp_gmf_audio_el_deinit(self);
        esp_gmf_oal_free(alc);
    }
    return ESP_GMF_ERR_OK;
}

esp_gmf_err_t esp_gmf_alc_set_gain(esp_gmf_audio_element_handle_t handle, uint8_t idx, int8_t gain)
{
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_gmf_method_t *method_head = NULL;
    esp_gmf_method_t *method = NULL;
    esp_gmf_element_get_method((esp_gmf_element_handle_t)handle, &method_head);
    esp_gmf_method_found(method_head, ESP_GMF_METHOD_ALC_SET_GAIN, &method);
    uint8_t buf[2] = {0};
    esp_gmf_args_set_value(method->args_desc, ESP_GMF_METHOD_ALC_SET_GAIN_ARG_IDX, buf, (uint8_t *)&idx, sizeof(idx));
    esp_gmf_args_set_value(method->args_desc, ESP_GMF_METHOD_ALC_SET_GAIN_ARG_GAIN, buf, (uint8_t *)&gain, sizeof(gain));
    return esp_gmf_element_exe_method((esp_gmf_element_handle_t)handle, ESP_GMF_METHOD_ALC_SET_GAIN, buf, sizeof(buf));
}

esp_gmf_err_t esp_gmf_alc_get_gain(esp_gmf_audio_element_handle_t handle, uint8_t idx, int8_t *gain)
{
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, gain, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_gmf_method_t *method_head = NULL;
    esp_gmf_method_t *method = NULL;
    esp_gmf_element_get_method((esp_gmf_element_handle_t)handle, &method_head);
    esp_gmf_method_found(method_head, ESP_GMF_METHOD_ALC_GET_GAIN, &method);
    uint8_t buf[2] = {0};
    esp_gmf_args_set_value(method->args_desc, ESP_GMF_METHOD_ALC_GET_GAIN_ARG_IDX, buf, (uint8_t *)&idx, sizeof(idx));
    esp_gmf_err_t ret = esp_gmf_element_exe_method((esp_gmf_element_handle_t)handle, ESP_GMF_METHOD_ALC_GET_GAIN, buf, sizeof(buf));
    if (ret != ESP_GMF_ERR_OK) {
        return ESP_GMF_ERR_FAIL;
    }
    esp_gmf_args_extract_value(method->args_desc,ESP_GMF_METHOD_ALC_GET_GAIN_ARG_GAIN, buf, sizeof(buf), (uint32_t *)gain);
    return ret;
}

esp_gmf_err_t esp_gmf_alc_init(esp_ae_alc_cfg_t *config, esp_gmf_obj_handle_t *handle)
{
    ESP_GMF_NULL_CHECK(TAG, config, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    *handle = NULL;
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
    esp_gmf_alc_t *alc = esp_gmf_oal_calloc(1, sizeof(esp_gmf_alc_t));
    ESP_GMF_MEM_VERIFY(TAG, alc, {return ESP_GMF_ERR_MEMORY_LACK;}, "ALC", sizeof(esp_gmf_alc_t));
    esp_gmf_obj_t *obj = (esp_gmf_obj_t *)alc;
    obj->new_obj = esp_gmf_alc_new;
    obj->del_obj = esp_gmf_alc_destroy;
    esp_ae_alc_cfg_t *cfg = esp_gmf_oal_calloc(1, sizeof(*config));
    ESP_GMF_MEM_VERIFY(TAG, cfg, {ret = ESP_GMF_ERR_MEMORY_LACK; goto ALC_INIT_FAIL;}, "alc configuration", sizeof(*config));
    memcpy(cfg, config, sizeof(*config));
    esp_gmf_obj_set_config(obj, cfg, sizeof(*config));
    ret = esp_gmf_obj_set_tag(obj, "alc");
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, goto ALC_INIT_FAIL, "Failed to set obj tag");
    esp_gmf_element_cfg_t el_cfg = {0};
    ESP_GMF_ELEMENT_IN_PORT_ATTR_SET(el_cfg.in_attr, ESP_GMF_EL_PORT_CAP_SINGLE, 0, 0,
                                    ESP_GMF_PORT_TYPE_BLOCK | ESP_GMF_PORT_TYPE_BYTE, ESP_GMF_ELEMENT_PORT_DATA_SIZE_DEFAULT);
    ESP_GMF_ELEMENT_OUT_PORT_ATTR_SET(el_cfg.out_attr, ESP_GMF_EL_PORT_CAP_SINGLE, 0, 0,
                                    ESP_GMF_PORT_TYPE_BLOCK | ESP_GMF_PORT_TYPE_BYTE, ESP_GMF_ELEMENT_PORT_DATA_SIZE_DEFAULT);
    el_cfg.dependency = true;
    ret = esp_gmf_audio_el_init(alc, &el_cfg);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, goto ALC_INIT_FAIL, "Failed to initialize alc element");
    *handle = obj;
    ESP_LOGD(TAG, "Initialization, %s-%p", OBJ_GET_TAG(obj), obj);
    return ESP_GMF_ERR_OK;
ALC_INIT_FAIL:
    esp_gmf_obj_delete(obj);
    return ret;
}

esp_gmf_err_t esp_gmf_alc_cast(esp_ae_alc_cfg_t *config, esp_gmf_obj_handle_t handle)
{
    ESP_GMF_NULL_CHECK(TAG, config, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_ae_alc_cfg_t *cfg = esp_gmf_oal_calloc(1, sizeof(*config));
    ESP_GMF_MEM_VERIFY(TAG, cfg, {return ESP_GMF_ERR_MEMORY_LACK;}, "alc configuration", sizeof(*config));
    memcpy(cfg, config, sizeof(*config));
    // Free memory before overwriting
    esp_gmf_oal_free(OBJ_GET_CFG(handle));
    esp_gmf_obj_set_config(handle, cfg, sizeof(*config));
    esp_gmf_audio_element_t *alc_el = (esp_gmf_audio_element_t *)handle;
    esp_gmf_args_desc_t *set_args = NULL;
    esp_gmf_args_desc_t *get_args = NULL;

    esp_gmf_err_t ret = esp_gmf_args_desc_append(&set_args, ESP_GMF_METHOD_ALC_SET_GAIN_ARG_IDX,
                                                 ESP_GMF_ARGS_TYPE_UINT8, sizeof(uint8_t), 0);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, {return ret;}, "Failed to append argument");
    ret = esp_gmf_args_desc_append(&set_args, ESP_GMF_METHOD_ALC_SET_GAIN_ARG_GAIN, ESP_GMF_ARGS_TYPE_INT8,
                                   sizeof(int8_t), sizeof(uint8_t));
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, {return ret;}, "Failed to append argument");
    ret = esp_gmf_element_register_method(alc_el, ESP_GMF_METHOD_ALC_SET_GAIN, __alc_set_gain, set_args);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, {return ret;}, "Failed to register method");

    ret = esp_gmf_args_desc_copy(set_args, &get_args);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, {return ret;}, "Failed to copy argument");
    ret = esp_gmf_element_register_method(alc_el, ESP_GMF_METHOD_ALC_GET_GAIN, __alc_get_gain, get_args);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, {return ret;}, "Failed to register method");

    alc_el->base.ops.open = esp_gmf_alc_open;
    alc_el->base.ops.process = esp_gmf_alc_process;
    alc_el->base.ops.close = esp_gmf_alc_close;
    alc_el->base.ops.event_receiver = alc_received_event_handler;
    return ESP_GMF_ERR_OK;
}
