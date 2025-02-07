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
#include "esp_gmf_fade.h"
#include "gmf_audio_common.h"
#include "esp_gmf_audio_method_def.h"

/**
 * @brief Audio fade context in GMF
 */
typedef struct {
    esp_gmf_audio_element_t  parent;           /*!< The GMF fade handle */
    esp_ae_fade_handle_t     fade_hd;          /*!< The audio effects fade handle */
    uint8_t                  bytes_per_sample; /*!< Bytes number of per sampling point */
    bool                     is_mode_changed;  /*!< The flag of whether fade mode is changed */
    bool                     is_fade_reset;    /*!< The flag of whether fade weight is reset */
    esp_ae_fade_mode_t       mode;             /*!< The current fade mode */
} esp_gmf_fade_t;

static const char *TAG = "ESP_GMF_FADE";

static inline esp_gmf_job_err_t fade_update_apply_setting(esp_gmf_fade_t *fade)
{
    esp_gmf_job_err_t ret = ESP_GMF_JOB_ERR_OK;
    if (fade->is_mode_changed == true) {
        fade->is_mode_changed = false;
        ret = esp_ae_fade_set_mode(fade->fade_hd, fade->mode);
        if (ret != ESP_GMF_JOB_ERR_OK) {
            return ESP_GMF_JOB_ERR_FAIL;
        }
    }
    if (fade->is_fade_reset == true) {
        fade->is_fade_reset = false;
        ret = esp_ae_fade_reset_weight(fade->fade_hd);
        if (ret != ESP_GMF_JOB_ERR_OK) {
            return ESP_GMF_JOB_ERR_FAIL;
        }
    }
    return ret;
}

static inline void fade_change_src_info(esp_gmf_audio_element_handle_t self, uint32_t src_rate, uint8_t src_ch, uint8_t src_bits)
{
    esp_ae_fade_cfg_t *fade_info = (esp_ae_fade_cfg_t *)OBJ_GET_CFG(self);
    fade_info->channel = src_ch;
    fade_info->sample_rate = src_rate;
    fade_info->bits_per_sample = src_bits;
}

static esp_gmf_err_t __fade_set_mode(esp_gmf_audio_element_handle_t handle, esp_gmf_args_desc_t *arg_desc,
                                     uint8_t *buf, int buf_len)
{
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, arg_desc, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, buf, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_gmf_fade_t *fade = (esp_gmf_fade_t *)handle;
    memcpy(&fade->mode, buf, arg_desc->size);
    fade->is_mode_changed = true;
    return ESP_GMF_ERR_OK;
}

static esp_gmf_err_t __fade_get_mode(esp_gmf_audio_element_handle_t handle, esp_gmf_args_desc_t *arg_desc,
                                     uint8_t *buf, int buf_len)
{
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, arg_desc, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, buf, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_gmf_fade_t *fade = (esp_gmf_fade_t *)handle;
    esp_ae_fade_mode_t *mode = (esp_ae_fade_mode_t *)buf;
    esp_ae_err_t ret = esp_ae_fade_get_mode(fade->fade_hd, mode);
    if (ret != ESP_AE_ERR_OK) {
        return ESP_GMF_ERR_FAIL;
    }
    return ESP_GMF_ERR_OK;
}

static esp_gmf_err_t __fade_reset(esp_gmf_audio_element_handle_t handle, esp_gmf_args_desc_t *arg_desc,
                                       uint8_t *buf, int buf_len)
{
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_gmf_fade_t *fade = (esp_gmf_fade_t *)handle;
    fade->is_fade_reset = true;
    return ESP_GMF_ERR_OK;
}

static esp_gmf_err_t esp_gmf_fade_new(void *cfg, esp_gmf_obj_handle_t *handle)
{
    ESP_GMF_NULL_CHECK(TAG, cfg, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    *handle = NULL;
    esp_ae_fade_cfg_t *fade_cfg = (esp_ae_fade_cfg_t *)cfg;
    esp_gmf_obj_handle_t new_obj = NULL;
    esp_gmf_err_t ret = esp_gmf_fade_init(fade_cfg, &new_obj);
    if (ret != ESP_GMF_ERR_OK) {
        return ret;
    }
    ret = esp_gmf_fade_cast(fade_cfg, new_obj);
    if (ret != ESP_GMF_ERR_OK) {
        esp_gmf_obj_delete(new_obj);
        return ret;
    }
    *handle = (void *)new_obj;
    return ret;
}

static esp_gmf_job_err_t esp_gmf_fade_open(esp_gmf_audio_element_handle_t self, void *para)
{
    ESP_GMF_NULL_CHECK(TAG, self, {return ESP_GMF_JOB_ERR_FAIL;});
    esp_gmf_fade_t *fade = (esp_gmf_fade_t *)self;
    esp_ae_fade_cfg_t *fade_info = (esp_ae_fade_cfg_t *)OBJ_GET_CFG(self);
    ESP_GMF_NULL_CHECK(TAG, fade_info, {return ESP_GMF_JOB_ERR_FAIL;});
    fade->bytes_per_sample = (fade_info->bits_per_sample >> 3) * fade_info->channel;
    esp_ae_fade_open(fade_info, &fade->fade_hd);
    ESP_GMF_CHECK(TAG, fade->fade_hd, {return ESP_GMF_JOB_ERR_FAIL;}, "Failed to create fade handle");
    GMF_AUDIO_UPDATE_SND_INFO(self, fade_info->sample_rate, fade_info->bits_per_sample, fade_info->channel);
    fade->is_mode_changed = false;
    fade->is_fade_reset = false;
    ESP_LOGD(TAG, "Open, %p", self);
    return ESP_GMF_ERR_OK;
}

static esp_gmf_job_err_t esp_gmf_fade_process(esp_gmf_audio_element_handle_t self, void *para)
{
    ESP_GMF_NULL_CHECK(TAG, self, {return ESP_GMF_JOB_ERR_FAIL;});
    esp_gmf_fade_t *fade = (esp_gmf_fade_t *)self;
    int out_len = -1;
    ESP_GMF_RET_ON_NOT_OK(TAG, fade_update_apply_setting(fade), {return ESP_GMF_JOB_ERR_FAIL;}, "Failed to apply fade setting");
    esp_gmf_port_handle_t in_port = ESP_GMF_ELEMENT_GET(self)->in;
    esp_gmf_port_handle_t out_port = ESP_GMF_ELEMENT_GET(self)->out;
    esp_gmf_payload_t *in_load = NULL;
    esp_gmf_payload_t *out_load = NULL;
    esp_gmf_err_io_t load_ret = esp_gmf_port_acquire_in(in_port, &in_load, GMF_AUDIO_INPUT_SAMPLE_NUM * fade->bytes_per_sample, ESP_GMF_MAX_DELAY);
    ESP_GMF_PORT_ACQUIRE_IN_CHECK(TAG, load_ret, out_len, {goto __fade_release;});
    int samples_num = in_load->valid_size / fade->bytes_per_sample;
    if (in_port->is_shared == 1) {
        out_load = in_load;
    }
    load_ret = esp_gmf_port_acquire_out(out_port, &out_load, samples_num ? samples_num * fade->bytes_per_sample : in_load->buf_length, ESP_GMF_MAX_DELAY);
    ESP_GMF_PORT_ACQUIRE_OUT_CHECK(TAG, load_ret, out_len, {goto __fade_release;});
    if (samples_num > 0) {
        esp_ae_err_t ret = esp_ae_fade_process(fade->fade_hd, samples_num, in_load->buf, out_load->buf);
        ESP_GMF_RET_ON_ERROR(TAG, ret, {out_len = ESP_GMF_JOB_ERR_FAIL; goto __fade_release;}, "Fade process error %d", ret);
    }
    ESP_LOGV(TAG, "Samples: %d, IN-PLD: %p-%p-%d-%d-%d, OUT-PLD: %p-%p-%d-%d-%d",
             samples_num, in_load, in_load->buf, in_load->valid_size, in_load->buf_length, in_load->is_done,
             out_load, out_load->buf, out_load->valid_size, out_load->buf_length, out_load->is_done);
    out_load->valid_size = samples_num * fade->bytes_per_sample;
    out_load->is_done = in_load->is_done;
    out_len = out_load->valid_size;
    if (out_len > 0) {
        esp_gmf_audio_el_update_file_pos((esp_gmf_element_handle_t)self, out_len);
    }
    if (in_load->is_done) {
        out_len = ESP_GMF_JOB_ERR_DONE;
        ESP_LOGD(TAG, "Fade done, out len: %d", out_load->valid_size);
    }
__fade_release:
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

static esp_gmf_job_err_t esp_gmf_fade_close(esp_gmf_audio_element_handle_t self, void *para)
{
    ESP_GMF_NULL_CHECK(TAG, self, {return ESP_GMF_ERR_OK;});
    esp_gmf_fade_t *fade = (esp_gmf_fade_t *)self;
    ESP_LOGD(TAG, "Closed, %p", self);
    if (fade->fade_hd != NULL) {
        esp_ae_fade_close(fade->fade_hd);
        fade->fade_hd = NULL;
    }
    return ESP_GMF_ERR_OK;
}

static esp_gmf_err_t fade_received_event_handler(esp_gmf_event_pkt_t *evt, void *ctx)
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
            fade_change_src_info(self, info.sample_rates, info.channels, info.bits);
            ESP_LOGD(TAG, "RECV element info, from: %s-%p, next: %p, self: %s-%p, type: %x, state: %s, rate: %d, ch: %d, bits: %d",
                     OBJ_GET_TAG(el), el, esp_gmf_node_for_next((esp_gmf_node_t *)el), OBJ_GET_TAG(self), self, evt->type,
                     esp_gmf_event_get_state_str(state), info.sample_rates, info.channels, info.bits);
            // Change the state to ESP_GMF_EVENT_STATE_INITIALIZED, then add to working list.
            esp_gmf_element_set_state(self, ESP_GMF_EVENT_STATE_INITIALIZED);
        }
    }
    return ESP_GMF_ERR_OK;
}

static esp_gmf_err_t esp_gmf_fade_destroy(esp_gmf_audio_element_handle_t self)
{
    if (self != NULL) {
        esp_gmf_fade_t *fade = (esp_gmf_fade_t *)self;
        ESP_LOGD(TAG, "Destroyed, %p", self);
        esp_gmf_oal_free(OBJ_GET_CFG(self));
        esp_gmf_audio_el_deinit(self);
        esp_gmf_oal_free(fade);
    }
    return ESP_GMF_ERR_OK;
}

esp_gmf_err_t esp_gmf_fade_set_mode(esp_gmf_audio_element_handle_t handle, esp_ae_fade_mode_t mode)
{
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_gmf_method_t *method_head = NULL;
    esp_gmf_method_t *method = NULL;
    esp_gmf_element_get_method((esp_gmf_element_handle_t)handle, &method_head);
    esp_gmf_method_found(method_head, ESP_GMF_METHOD_FADE_SET_MODE, &method);
    uint8_t buf[4] = {0};
    esp_gmf_args_set_value(method->args_desc, ESP_GMF_METHOD_FADE_SET_MODE_ARG_MODE, buf, (uint8_t *)&mode, sizeof(mode));
    return esp_gmf_element_exe_method((esp_gmf_element_handle_t)handle, ESP_GMF_METHOD_FADE_SET_MODE, buf, sizeof(buf));
}

esp_gmf_err_t esp_gmf_fade_get_mode(esp_gmf_audio_element_handle_t handle, esp_ae_fade_mode_t *mode)
{
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_gmf_method_t *method_head = NULL;
    esp_gmf_method_t *method = NULL;
    esp_gmf_element_get_method((esp_gmf_element_handle_t)handle, &method_head);
    esp_gmf_method_found(method_head, ESP_GMF_METHOD_FADE_GET_MODE, &method);
    uint8_t buf[4] = {0};
    esp_gmf_err_t ret = esp_gmf_element_exe_method((esp_gmf_element_handle_t)handle, ESP_GMF_METHOD_FADE_GET_MODE, buf, sizeof(buf));
    if (ret != ESP_GMF_ERR_OK) {
        return ESP_GMF_ERR_FAIL;
    }
    esp_gmf_args_extract_value(method->args_desc, ESP_GMF_METHOD_FADE_GET_MODE_ARG_MODE, buf, sizeof(buf), (uint32_t *)mode);
    return ret;
}

esp_gmf_err_t esp_gmf_fade_reset(esp_gmf_audio_element_handle_t handle)
{
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_gmf_method_t *method_head = NULL;
    esp_gmf_method_t *method = NULL;
    esp_gmf_element_get_method((esp_gmf_element_handle_t)handle, &method_head);
    esp_gmf_method_found(method_head, ESP_GMF_METHOD_FADE_RESET, &method);
    uint8_t buf[1] = {0};
    return esp_gmf_element_exe_method((esp_gmf_element_handle_t)handle, ESP_GMF_METHOD_FADE_RESET, buf, sizeof(buf));
}

esp_gmf_err_t esp_gmf_fade_init(esp_ae_fade_cfg_t *config, esp_gmf_obj_handle_t *handle)
{
    ESP_GMF_NULL_CHECK(TAG, config, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    *handle = NULL;
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
    esp_gmf_fade_t *fade = esp_gmf_oal_calloc(1, sizeof(esp_gmf_fade_t));
    ESP_GMF_MEM_VERIFY(TAG, fade, {return ESP_GMF_ERR_MEMORY_LACK;}, "fade", sizeof(esp_gmf_fade_t));
    esp_gmf_obj_t *obj = (esp_gmf_obj_t *)fade;
    obj->new_obj = esp_gmf_fade_new;
    obj->del_obj = esp_gmf_fade_destroy;
    esp_ae_fade_cfg_t *cfg = esp_gmf_oal_calloc(1, sizeof(*config));
    ESP_GMF_MEM_VERIFY(TAG, cfg, {ret = ESP_GMF_ERR_MEMORY_LACK; goto FADE_INIT_FAIL;}, "fade configuration", sizeof(*config));
    memcpy(cfg, config, sizeof(*config));
    esp_gmf_obj_set_config(obj, cfg, sizeof(*config));
    ret = esp_gmf_obj_set_tag(obj, "fade");
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, goto FADE_INIT_FAIL, "Failed to set obj tag");
    esp_gmf_element_cfg_t el_cfg = {0};
    ESP_GMF_ELEMENT_CFG(el_cfg, true, ESP_GMF_EL_PORT_CAP_SINGLE, ESP_GMF_EL_PORT_CAP_SINGLE,
                        ESP_GMF_PORT_TYPE_BLOCK | ESP_GMF_PORT_TYPE_BYTE, ESP_GMF_PORT_TYPE_BYTE | ESP_GMF_PORT_TYPE_BLOCK);
    ret = esp_gmf_audio_el_init(fade, &el_cfg);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, goto FADE_INIT_FAIL, "Failed to initialize fade element");
    *handle = obj;
    ESP_LOGD(TAG, "Initialization, %s-%p", OBJ_GET_TAG(obj), obj);
    return ESP_GMF_ERR_OK;
FADE_INIT_FAIL:
    esp_gmf_obj_delete(obj);
    return ret;
}

esp_gmf_err_t esp_gmf_fade_cast(esp_ae_fade_cfg_t *config, esp_gmf_obj_handle_t handle)
{
    ESP_GMF_NULL_CHECK(TAG, config, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
    esp_ae_fade_cfg_t *cfg = esp_gmf_oal_calloc(1, sizeof(*config));
    ESP_GMF_MEM_VERIFY(TAG, cfg, {return ESP_GMF_ERR_MEMORY_LACK;}, "fade configuration", sizeof(*config));
    memcpy(cfg, config, sizeof(*config));
    // Free memory before overwriting
    esp_gmf_oal_free(OBJ_GET_CFG(handle));
    esp_gmf_obj_set_config(handle, cfg, sizeof(*config));
    esp_gmf_audio_element_t *fade_el = (esp_gmf_audio_element_t *)handle;
    esp_gmf_args_desc_t *set_args = NULL;
    esp_gmf_args_desc_t *get_args = NULL;

    ret = esp_gmf_args_desc_append(&set_args, ESP_GMF_METHOD_FADE_SET_MODE_ARG_MODE, ESP_GMF_ARGS_TYPE_INT32, sizeof(int32_t), 0);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, {return ret;}, "Failed to append argument");
    ret = esp_gmf_element_register_method(fade_el, ESP_GMF_METHOD_FADE_SET_MODE, __fade_set_mode, set_args);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, {return ret;}, "Failed to register method");

    ret = esp_gmf_args_desc_copy(set_args, &get_args);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, {return ret;}, "Failed to copy argument");
    ret = esp_gmf_element_register_method(fade_el, ESP_GMF_METHOD_FADE_GET_MODE, __fade_get_mode, get_args);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, {return ret;}, "Failed to register method");

    ret = esp_gmf_element_register_method(fade_el, ESP_GMF_METHOD_FADE_RESET, __fade_reset, NULL);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, {return ret;}, "Failed to register method");

    fade_el->base.ops.open = esp_gmf_fade_open;
    fade_el->base.ops.process = esp_gmf_fade_process;
    fade_el->base.ops.close = esp_gmf_fade_close;
    fade_el->base.ops.event_receiver = fade_received_event_handler;
    return ESP_GMF_ERR_OK;
}
