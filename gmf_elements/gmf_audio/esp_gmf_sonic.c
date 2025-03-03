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
#include "esp_gmf_sonic.h"
#include "gmf_audio_common.h"
#include "esp_gmf_audio_method_def.h"

#define SONIC_DEFAULT_OUTPUT_TIME_MS (10)

/**
 * @brief Audio sonic context in GMF
 */
typedef struct {
    esp_gmf_audio_element_t parent;           /*!< The GMF sonic handle */
    esp_ae_sonic_handle_t   sonic_hd;         /*!< The audio effects sonic handle */
    uint8_t                 bytes_per_sample; /*!< Bytes number of per sampling point */
    uint32_t                sample_rate;      /*!< The audio sample rate */
    uint8_t                 bits_per_sample;  /*!< Bits number of per sampling point */
    uint8_t                 channel;          /*!< The audio channel */
    esp_ae_sonic_in_data_t  in_data_hd;       /*!< The sonic input data handle */
    esp_ae_sonic_out_data_t out_data_hd;      /*!< The sonic output data handle */
    float                   speed;            /*!< The audio speed */
    float                   pitch;            /*!< The audio pitch */
    int32_t                 out_size;         /*!< The acquired out size */
    bool                    is_speed_change;  /*!< The flag of whether speed is changed */
    bool                    is_pitch_change;  /*!< The flag of whether pitch is changed */
} esp_gmf_sonic_t;

static const char *TAG = "ESP_GMF_SONIC";

static inline esp_gmf_job_err_t sonic_update_apply_setting(esp_gmf_sonic_t *sonic)
{
    esp_gmf_job_err_t ret = ESP_GMF_JOB_ERR_OK;
    if (sonic->is_pitch_change == true) {
        sonic->is_pitch_change = false;
        ret = esp_ae_sonic_set_pitch(sonic->sonic_hd, sonic->pitch);
        if (ret != ESP_GMF_JOB_ERR_OK) {
            return ESP_GMF_JOB_ERR_FAIL;
        }
    }
    if (sonic->is_speed_change == true) {
        sonic->is_speed_change = false;
        ret = esp_ae_sonic_set_speed(sonic->sonic_hd, sonic->speed);
        if (ret != ESP_GMF_JOB_ERR_OK) {
            return ESP_GMF_JOB_ERR_FAIL;
        }
    }
    return ret;
}

static inline void sonic_change_src_info(esp_gmf_audio_element_handle_t self, uint32_t src_rate, uint8_t src_ch, uint8_t src_bits)
{
    esp_ae_sonic_cfg_t *sonic_info = (esp_ae_sonic_cfg_t *)OBJ_GET_CFG(self);
    sonic_info->channel = src_ch;
    sonic_info->sample_rate = src_rate;
    sonic_info->bits_per_sample = src_bits;
}

static esp_gmf_err_t __sonic_set_speed(esp_gmf_audio_element_handle_t handle, esp_gmf_args_desc_t *arg_desc,
                                       uint8_t *buf, int buf_len)
{
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, arg_desc, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, buf, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_gmf_sonic_t *sonic = (esp_gmf_sonic_t *)handle;
    memcpy(&sonic->speed, buf, arg_desc->size);
    sonic->is_speed_change = true;
    return ESP_GMF_ERR_OK;
}

static esp_gmf_err_t __sonic_get_speed(esp_gmf_audio_element_handle_t handle, esp_gmf_args_desc_t *arg_desc,
                                       uint8_t *buf, int buf_len)
{
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, arg_desc, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, buf, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_gmf_sonic_t *sonic = (esp_gmf_sonic_t *)handle;
    float *speed = (float *)buf;
    esp_ae_err_t ret = esp_ae_sonic_get_speed(sonic->sonic_hd, speed);
    if (ret != ESP_AE_ERR_OK) {
        return ESP_GMF_ERR_FAIL;
    }
    return ESP_GMF_ERR_OK;
}

static esp_gmf_err_t __sonic_set_pitch(esp_gmf_audio_element_handle_t handle, esp_gmf_args_desc_t *arg_desc,
                                       uint8_t *buf, int buf_len)
{
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, arg_desc, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, buf, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_gmf_sonic_t *sonic = (esp_gmf_sonic_t *)handle;
    memcpy(&sonic->pitch, buf, arg_desc->size);
    sonic->is_pitch_change = true;
    return ESP_GMF_ERR_OK;
}

static esp_gmf_err_t __sonic_get_pitch(esp_gmf_audio_element_handle_t handle, esp_gmf_args_desc_t *arg_desc,
                                       uint8_t *buf, int buf_len)
{
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, arg_desc, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, buf, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_gmf_sonic_t *sonic = (esp_gmf_sonic_t *)handle;
    float *pitch = (float *)buf;
    esp_ae_err_t ret = esp_ae_sonic_get_pitch(sonic->sonic_hd, pitch);
    if (ret != ESP_AE_ERR_OK) {
        return ESP_GMF_ERR_FAIL;
    }
    return ESP_GMF_ERR_OK;
}

static esp_gmf_err_t esp_gmf_sonic_new(void *cfg, esp_gmf_obj_handle_t *handle)
{
    ESP_GMF_NULL_CHECK(TAG, cfg, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    *handle = NULL;
    esp_ae_sonic_cfg_t *sonic_cfg = (esp_ae_sonic_cfg_t *)cfg;
    esp_gmf_obj_handle_t new_obj = NULL;
    esp_gmf_err_t ret = esp_gmf_sonic_init(sonic_cfg, &new_obj);
    if (ret != ESP_GMF_ERR_OK) {
        return ret;
    }
    ret = esp_gmf_sonic_cast(sonic_cfg, new_obj);
    if (ret != ESP_GMF_ERR_OK) {
        esp_gmf_obj_delete(new_obj);
        return ret;
    }
    *handle = (void *)new_obj;
    return ret;
}

static esp_gmf_job_err_t esp_gmf_sonic_open(esp_gmf_audio_element_handle_t self, void *para)
{
    esp_gmf_sonic_t *sonic = (esp_gmf_sonic_t *)self;
    esp_ae_sonic_cfg_t *sonic_info = (esp_ae_sonic_cfg_t *)OBJ_GET_CFG(self);
    ESP_GMF_NULL_CHECK(TAG, sonic_info, {return ESP_GMF_JOB_ERR_FAIL;});
    sonic->sample_rate = sonic_info->sample_rate;
    sonic->channel = sonic_info->channel;
    sonic->bits_per_sample = sonic_info->bits_per_sample;
    sonic->bytes_per_sample = (sonic_info->bits_per_sample >> 3) * sonic_info->channel;
    sonic->out_size = SONIC_DEFAULT_OUTPUT_TIME_MS * sonic->sample_rate * sonic->bytes_per_sample / 1000;
    esp_ae_sonic_open(sonic_info, &sonic->sonic_hd);
    ESP_GMF_CHECK(TAG, sonic->sonic_hd, {return ESP_GMF_JOB_ERR_FAIL;}, "Failed to create sonic handle");
    sonic->is_pitch_change = false;
    sonic->is_speed_change = false;
    ESP_LOGD(TAG, "Open, %p", self);
    return ESP_GMF_JOB_ERR_OK;
}

static esp_gmf_job_err_t esp_gmf_sonic_process(esp_gmf_audio_element_handle_t self, void *para)
{
    ESP_GMF_NULL_CHECK(TAG, self, {return ESP_GMF_JOB_ERR_FAIL;});
    esp_gmf_sonic_t *sonic = (esp_gmf_sonic_t *)self;
    int out_len = -1;
    esp_ae_err_t ret = ESP_AE_ERR_OK;
    esp_gmf_port_handle_t in_port = ESP_GMF_ELEMENT_GET(self)->in;
    esp_gmf_port_handle_t out_port = ESP_GMF_ELEMENT_GET(self)->out;
    esp_gmf_payload_t *in_load = NULL;
    esp_gmf_payload_t *out_load = NULL;
    ESP_GMF_RET_ON_NOT_OK(TAG, sonic_update_apply_setting(sonic), {return ESP_GMF_JOB_ERR_FAIL;}, "Failed to apply sonic setting");
    esp_gmf_err_io_t load_ret = esp_gmf_port_acquire_in(in_port, &in_load, ESP_GMF_ELEMENT_GET(sonic)->in_attr.data_size, ESP_GMF_MAX_DELAY);
    ESP_GMF_PORT_ACQUIRE_IN_CHECK(TAG, load_ret, out_len, {goto __sonic_release;});
    sonic->in_data_hd.samples = in_load->buf;
    sonic->in_data_hd.num = in_load->valid_size / (sonic->bytes_per_sample);
    char *in = sonic->in_data_hd.samples;
    int64_t pts = in_load->pts;
    int64_t frame_dur_ms = 0;
    while (sonic->in_data_hd.num > 0 || sonic->out_data_hd.out_num > 0) {
        load_ret = esp_gmf_port_acquire_out(out_port, &out_load, sonic->out_size, ESP_GMF_MAX_DELAY);
        ESP_GMF_PORT_ACQUIRE_OUT_CHECK(TAG, load_ret, out_len, {goto __sonic_release;});
        sonic->out_data_hd.needed_num = sonic->out_size / sonic->bytes_per_sample;
        sonic->out_data_hd.samples = out_load->buf;
        ret = esp_ae_sonic_process(sonic->sonic_hd, &sonic->in_data_hd, &sonic->out_data_hd);
        ESP_GMF_RET_ON_ERROR(TAG, ret, {out_len = ESP_GMF_JOB_ERR_FAIL; goto __sonic_release;}, "Sonic process error %d", ret);
        out_load->valid_size = sonic->out_data_hd.out_num * sonic->bytes_per_sample;
        out_load->pts = pts;
        out_load->is_done = in_load->is_done;
        frame_dur_ms = (int64_t)((float)(out_load->valid_size * 8000 / (sonic->sample_rate * sonic->bits_per_sample * sonic->channel)) * sonic->speed);
        pts += frame_dur_ms;
        out_len = out_load->valid_size;
        if (out_len > 0) {
            esp_gmf_audio_el_update_file_pos((esp_gmf_element_handle_t)self, out_len);
        }
        ESP_LOGV(TAG, "%s, I: %p-buf: %p-sz: %d, O: %p-buf: %p-sz: %d, ret: %d", __func__, in_port,
                 in_load->buf, in_load->valid_size, out_port,
                 out_load->buf, out_load->buf_length, ret);
        load_ret = esp_gmf_port_release_out(out_port, out_load, ESP_GMF_MAX_DELAY);
        ESP_GMF_PORT_RELEASE_OUT_CHECK(TAG, load_ret, out_len, NULL);
        in = (char *)sonic->in_data_hd.samples + sonic->in_data_hd.consume_num * sonic->bytes_per_sample;
        sonic->in_data_hd.samples = in;
        sonic->in_data_hd.num -= sonic->in_data_hd.consume_num;
    }
    if (in_load->is_done) {
        out_len = ESP_GMF_JOB_ERR_DONE;
        ESP_LOGD(TAG, "Sonic done");
    }
__sonic_release:
    if (in_load != NULL) {
        load_ret = esp_gmf_port_release_in(in_port, in_load, ESP_GMF_MAX_DELAY);
        ESP_GMF_PORT_RELEASE_IN_CHECK(TAG, load_ret, out_len, NULL);
    }
    return out_len;
}

static esp_gmf_job_err_t esp_gmf_sonic_close(esp_gmf_audio_element_handle_t self, void *para)
{
    ESP_GMF_NULL_CHECK(TAG, self, {return ESP_GMF_ERR_OK;});
    esp_gmf_sonic_t *sonic = (esp_gmf_sonic_t *)self;
    ESP_LOGD(TAG, "Closed, %p", self);
    if (sonic->sonic_hd != NULL) {
        esp_ae_sonic_close(sonic->sonic_hd);
        sonic->sonic_hd = NULL;
    }
    return ESP_GMF_ERR_OK;
}

static esp_gmf_err_t sonic_received_event_handler(esp_gmf_event_pkt_t *evt, void *ctx)
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
            sonic_change_src_info(self, info.sample_rates, info.channels, info.bits);
            ESP_LOGD(TAG, "RECV element info, from: %s-%p, next: %p, self: %s-%p, type: %x, state: %s, rate: %d, ch: %d, bits: %d",
                     OBJ_GET_TAG(el), el, esp_gmf_node_for_next((esp_gmf_node_t *)el), OBJ_GET_TAG(self), self, evt->type,
                     esp_gmf_event_get_state_str(state), info.sample_rates, info.channels, info.bits);
            // Change the state to ESP_GMF_EVENT_STATE_INITIALIZED, then add to working list.
            esp_gmf_element_set_state(self, ESP_GMF_EVENT_STATE_INITIALIZED);
        }
    }
    return ESP_GMF_ERR_OK;
}

static esp_gmf_err_t esp_gmf_sonic_destroy(esp_gmf_audio_element_handle_t self)
{
    if (self != NULL) {
        esp_gmf_sonic_t *sonic = (esp_gmf_sonic_t *)self;
        ESP_LOGD(TAG, "Destroyed, %p", self);
        esp_gmf_oal_free(OBJ_GET_CFG(self));
        esp_gmf_audio_el_deinit(self);
        esp_gmf_oal_free(sonic);
    }
    return ESP_GMF_ERR_OK;
}

esp_gmf_err_t esp_gmf_sonic_set_speed(esp_gmf_audio_element_handle_t handle, float speed)
{
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_gmf_method_t *method_head = NULL;
    esp_gmf_method_t *method = NULL;
    esp_gmf_element_get_method((esp_gmf_element_handle_t)handle, &method_head);
    esp_gmf_method_found(method_head, ESP_GMF_METHOD_SONIC_SET_SPEED, &method);
    uint8_t buf[4] = {0};
    esp_gmf_args_set_value(method->args_desc, ESP_GMF_METHOD_SONIC_SET_SPEED_ARG_SPEED, buf, (uint8_t *)&speed, sizeof(speed));
    return esp_gmf_element_exe_method((esp_gmf_element_handle_t)handle, ESP_GMF_METHOD_SONIC_SET_SPEED, buf, sizeof(buf));
}

esp_gmf_err_t esp_gmf_sonic_get_speed(esp_gmf_audio_element_handle_t handle, float *speed)
{
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_gmf_method_t *method_head = NULL;
    esp_gmf_method_t *method = NULL;
    esp_gmf_element_get_method((esp_gmf_element_handle_t)handle, &method_head);
    esp_gmf_method_found(method_head, ESP_GMF_METHOD_SONIC_GET_SPEED, &method);
    uint8_t buf[4] = {0};
    esp_gmf_err_t ret = esp_gmf_element_exe_method((esp_gmf_element_handle_t)handle, ESP_GMF_METHOD_SONIC_GET_SPEED, buf, sizeof(buf));
    if (ret != ESP_GMF_ERR_OK) {
        return ESP_GMF_ERR_FAIL;
    }
    esp_gmf_args_extract_value(method->args_desc, ESP_GMF_METHOD_SONIC_GET_SPEED_ARG_SPEED, buf, sizeof(buf), (uint32_t *)speed);
    return ret;
}

esp_gmf_err_t esp_gmf_sonic_set_pitch(esp_gmf_audio_element_handle_t handle, float pitch)
{
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_gmf_method_t *method_head = NULL;
    esp_gmf_method_t *method = NULL;
    esp_gmf_element_get_method((esp_gmf_element_handle_t)handle, &method_head);
    esp_gmf_method_found(method_head, ESP_GMF_METHOD_SONIC_SET_PITCH, &method);
    uint8_t buf[4] = {0};
    esp_gmf_args_set_value(method->args_desc, ESP_GMF_METHOD_SONIC_SET_PITCH_ARG_PITCH, buf, (uint8_t *)&pitch, sizeof(pitch));
    return esp_gmf_element_exe_method((esp_gmf_element_handle_t)handle, ESP_GMF_METHOD_SONIC_SET_PITCH, buf, sizeof(buf));
}

esp_gmf_err_t esp_gmf_sonic_get_pitch(esp_gmf_audio_element_handle_t handle, float *pitch)
{
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_gmf_method_t *method_head = NULL;
    esp_gmf_method_t *method = NULL;
    esp_gmf_element_get_method((esp_gmf_element_handle_t)handle, &method_head);
    esp_gmf_method_found(method_head, ESP_GMF_METHOD_SONIC_GET_PITCH, &method);
    uint8_t buf[4] = {0};
    esp_gmf_err_t ret = esp_gmf_element_exe_method((esp_gmf_element_handle_t)handle, ESP_GMF_METHOD_SONIC_GET_PITCH, buf, sizeof(buf));
    if (ret != ESP_GMF_ERR_OK) {
        return ESP_GMF_ERR_FAIL;
    }
    esp_gmf_args_extract_value(method->args_desc, ESP_GMF_METHOD_SONIC_GET_PITCH_ARG_PITCH, buf, sizeof(buf), (uint32_t *)pitch);
    return ret;
}

esp_gmf_err_t esp_gmf_sonic_init(esp_ae_sonic_cfg_t *config, esp_gmf_obj_handle_t *handle)
{
    ESP_GMF_NULL_CHECK(TAG, config, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    *handle = NULL;
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
    esp_gmf_sonic_t *sonic = esp_gmf_oal_calloc(1, sizeof(esp_gmf_sonic_t));
    ESP_GMF_MEM_VERIFY(TAG, sonic, {return ESP_GMF_ERR_MEMORY_LACK;}, "sonic", sizeof(esp_gmf_sonic_t));
    esp_gmf_obj_t *obj = (esp_gmf_obj_t *)sonic;
    obj->new_obj = esp_gmf_sonic_new;
    obj->del_obj = esp_gmf_sonic_destroy;
    esp_ae_sonic_cfg_t *cfg = esp_gmf_oal_calloc(1, sizeof(*config));
    ESP_GMF_MEM_VERIFY(TAG, cfg, {ret = ESP_GMF_ERR_MEMORY_LACK; goto SONIC_INIT_FAIL;}, "sonic configuration", sizeof(*config));
    memcpy(cfg, config, sizeof(*config));
    esp_gmf_obj_set_config(obj, cfg, sizeof(*config));
    ret = esp_gmf_obj_set_tag(obj, "sonic");
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, goto SONIC_INIT_FAIL, "Failed to set obj tag");
    esp_gmf_element_cfg_t el_cfg = {0};
    ESP_GMF_ELEMENT_IN_PORT_ATTR_SET(el_cfg.in_attr, ESP_GMF_EL_PORT_CAP_SINGLE, 0, 0,
        ESP_GMF_PORT_TYPE_BLOCK | ESP_GMF_PORT_TYPE_BYTE, ESP_GMF_ELEMENT_PORT_DATA_SIZE_DEFAULT);
    ESP_GMF_ELEMENT_IN_PORT_ATTR_SET(el_cfg.out_attr, ESP_GMF_EL_PORT_CAP_SINGLE, 0, 0,
        ESP_GMF_PORT_TYPE_BLOCK | ESP_GMF_PORT_TYPE_BYTE, ESP_GMF_ELEMENT_PORT_DATA_SIZE_DEFAULT);
    el_cfg.dependency = true;
    ret = esp_gmf_audio_el_init(sonic, &el_cfg);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, goto SONIC_INIT_FAIL, "Failed to initialize sonic element");
    *handle = obj;
    ESP_LOGD(TAG, "Initialization, %s-%p", OBJ_GET_TAG(obj), obj);
    return ESP_GMF_ERR_OK;
SONIC_INIT_FAIL:
    esp_gmf_obj_delete(obj);
    return ret;
}

esp_gmf_err_t esp_gmf_sonic_cast(esp_ae_sonic_cfg_t *config, esp_gmf_obj_handle_t handle)
{
    ESP_GMF_NULL_CHECK(TAG, config, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_ae_sonic_cfg_t *cfg = esp_gmf_oal_calloc(1, sizeof(*config));
    ESP_GMF_MEM_VERIFY(TAG, cfg, {return ESP_GMF_ERR_MEMORY_LACK;}, "sonic configuration", sizeof(*config));
    memcpy(cfg, config, sizeof(*config));
    // Free memory before overwriting
    esp_gmf_oal_free(OBJ_GET_CFG(handle));
    esp_gmf_obj_set_config(handle, cfg, sizeof(*config));
    esp_gmf_audio_element_t *sonic_el = (esp_gmf_audio_element_t *)handle;
    esp_gmf_args_desc_t *set_args = NULL;
    esp_gmf_args_desc_t *get_args = NULL;

    esp_gmf_err_t ret = esp_gmf_args_desc_append(&set_args, ESP_GMF_METHOD_SONIC_SET_SPEED_ARG_SPEED,
                                                 ESP_GMF_ARGS_TYPE_FLOAT, sizeof(float), 0);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, {return ret;}, "Failed to append argument");
    ret = esp_gmf_element_register_method(sonic_el, ESP_GMF_METHOD_SONIC_SET_SPEED, __sonic_set_speed, set_args);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, {return ret;}, "Failed to register method");

    ret = esp_gmf_args_desc_copy(set_args, &get_args);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, {return ret;}, "Failed to copy argument");
    ret = esp_gmf_element_register_method(sonic_el, ESP_GMF_METHOD_SONIC_GET_SPEED, __sonic_get_speed, get_args);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, {return ret;}, "Failed to register method");

    set_args = NULL;
    get_args = NULL;
    ret = esp_gmf_args_desc_append(&set_args, ESP_GMF_METHOD_SONIC_SET_PITCH_ARG_PITCH, ESP_GMF_ARGS_TYPE_FLOAT, sizeof(float), 0);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, {return ret;}, "Failed to append argument");
    ret = esp_gmf_element_register_method(sonic_el, ESP_GMF_METHOD_SONIC_SET_PITCH, __sonic_set_pitch, set_args);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, {return ret;}, "Failed to register method");

    ret = esp_gmf_args_desc_copy(set_args, &get_args);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, {return ret;}, "Failed to copy argument");
    ret = esp_gmf_element_register_method(sonic_el, ESP_GMF_METHOD_SONIC_GET_PITCH, __sonic_get_pitch, get_args);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, {return ret;}, "Failed to register method");

    sonic_el->base.ops.open = esp_gmf_sonic_open;
    sonic_el->base.ops.process = esp_gmf_sonic_process;
    sonic_el->base.ops.close = esp_gmf_sonic_close;
    sonic_el->base.ops.event_receiver = sonic_received_event_handler;
    return ESP_GMF_ERR_OK;
}
