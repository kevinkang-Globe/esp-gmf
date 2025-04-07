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
#include "esp_gmf_eq.h"
#include "esp_gmf_args_desc.h"
#include "gmf_audio_common.h"
#include "esp_gmf_audio_method_def.h"

/**
 * @brief  Information temporarily stored after the user calls the eq set interface
 */
typedef struct {
    int8_t                  is_para_changed;   /*!< The flag of whether eq parameter is changed */
    int8_t                  is_filter_enabled; /*!< The flag of whether eq filter is enabled */
    int8_t                  filter_last_state; /*!< The last state of eq filter is enabled */
    esp_ae_eq_filter_para_t para;              /*!< The parameter of eq filter */
} esp_gmf_eq_set_info_t;

/**
 * @brief Audio equalizer context in GMF
 */
typedef struct {
    esp_gmf_audio_element_t  parent;           /*!< The GMF eq handle */
    esp_ae_eq_handle_t       eq_hd;            /*!< The audio effects eq handle */
    uint8_t                  bytes_per_sample; /*!< Bytes number of per sampling point */
    int8_t                   filter_num;       /*!< The filter number of equalizer */
    esp_gmf_eq_set_info_t   *set_info;         /*!< Changed information of eq */
} esp_gmf_eq_t;

static const char *TAG = "ESP_GMF_EQ";

const esp_ae_eq_filter_para_t esp_gmf_default_eq_paras[10] = {
    {ESP_AE_EQ_FILTER_PEAK, 31, 1.0, 0.0},
    {ESP_AE_EQ_FILTER_PEAK, 62, 1.0, 0.0},
    {ESP_AE_EQ_FILTER_PEAK, 125, 1.0, 0.0},
    {ESP_AE_EQ_FILTER_PEAK, 250, 1.0, 1.0},
    {ESP_AE_EQ_FILTER_PEAK, 500, 1.0, 2.0},
    {ESP_AE_EQ_FILTER_PEAK, 1000, 1.0, 3.0},
    {ESP_AE_EQ_FILTER_PEAK, 2000, 1.0, 3.0},
    {ESP_AE_EQ_FILTER_PEAK, 4000, 1.0, 2.0},
    {ESP_AE_EQ_FILTER_PEAK, 8000, 1.0, 1.0},
    {ESP_AE_EQ_FILTER_PEAK, 16000, 1.0, 0.0},
};

static inline esp_gmf_err_t dupl_esp_ae_eq_cfg(esp_ae_eq_cfg_t *config, esp_ae_eq_cfg_t **new_config)
{
    void *sub_cfg = NULL;
    *new_config = esp_gmf_oal_calloc(1, sizeof(*config));
    ESP_GMF_MEM_VERIFY(TAG, *new_config, {return ESP_GMF_ERR_MEMORY_LACK;}, "eq configuration", sizeof(*config));
    memcpy(*new_config, config, sizeof(*config));
    if (config->para && (config->filter_num > 0)) {
        sub_cfg = esp_gmf_oal_calloc(1, config->filter_num * sizeof(esp_ae_eq_filter_para_t));
        ESP_GMF_MEM_VERIFY(TAG, sub_cfg, {esp_gmf_oal_free(*new_config); return ESP_GMF_ERR_MEMORY_LACK;},
                           "filter parameter", config->filter_num * sizeof(esp_ae_eq_filter_para_t));
        memcpy(sub_cfg, config->para, config->filter_num * sizeof(esp_ae_eq_filter_para_t));
        (*new_config)->para = sub_cfg;
    }
    return ESP_GMF_JOB_ERR_OK;
}

static inline void free_esp_ae_eq_cfg(esp_ae_eq_cfg_t *config)
{
    if (config) {
        if (config->para && (config->para != esp_gmf_default_eq_paras)) {
            esp_gmf_oal_free(config->para);
        }
        esp_gmf_oal_free(config);
    }
}

static inline esp_gmf_job_err_t eq_update_apply_setting(esp_gmf_eq_t *eq)
{
    esp_ae_err_t ret = ESP_AE_ERR_OK;
    for (int32_t i = 0; i < eq->filter_num; i++) {
        if (eq->set_info[i].filter_last_state != eq->set_info[i].is_filter_enabled) {
            if (eq->set_info[i].is_filter_enabled == true) {
                ret = esp_ae_eq_enable_filter(eq->eq_hd, i);
                if (ret != ESP_AE_ERR_OK) {
                    return ESP_GMF_JOB_ERR_FAIL;
                }
            } else {
                ret = esp_ae_eq_disable_filter(eq->eq_hd, i);
                if (ret != ESP_AE_ERR_OK) {
                    return ESP_GMF_JOB_ERR_FAIL;
                }
            }
            eq->set_info[i].filter_last_state = eq->set_info[i].is_filter_enabled;
        }
        if (eq->set_info[i].is_para_changed == true) {
            eq->set_info[i].is_para_changed = false;
            ret = esp_ae_eq_set_filter_para(eq->eq_hd, i, &(eq->set_info[i].para));
            if (ret != ESP_AE_ERR_OK) {
                return ESP_GMF_JOB_ERR_FAIL;
            }
        }
    }
    return ESP_GMF_JOB_ERR_OK;
}

static inline void eq_change_src_info(esp_gmf_audio_element_handle_t self, uint32_t src_rate, uint8_t src_ch, uint8_t src_bits)
{
    esp_ae_eq_cfg_t *eq_info = (esp_ae_eq_cfg_t *)OBJ_GET_CFG(self);
    eq_info->channel = src_ch;
    eq_info->sample_rate = src_rate;
    eq_info->bits_per_sample = src_bits;
}

static esp_gmf_err_t __eq_set_para(esp_gmf_audio_element_handle_t handle, esp_gmf_args_desc_t *arg_desc,
                                   uint8_t *buf, int buf_len)
{
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, arg_desc, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, buf, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_gmf_eq_t *eq = (esp_gmf_eq_t *)handle;
    ESP_GMF_NULL_CHECK(TAG, eq->set_info, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_gmf_args_desc_t *filter_desc = arg_desc;
    uint8_t idx = (uint8_t)(*buf);
    filter_desc = filter_desc->next;
    memcpy(&(eq->set_info[idx].para), buf + filter_desc->offset, filter_desc->size);
    eq->set_info[idx].is_para_changed = true;
    return ESP_GMF_ERR_OK;
}

static esp_gmf_err_t __eq_get_para(esp_gmf_audio_element_handle_t handle, esp_gmf_args_desc_t *arg_desc,
                                   uint8_t *buf, int buf_len)
{
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, arg_desc, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, buf, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_gmf_eq_t *eq = (esp_gmf_eq_t *)handle;
    esp_gmf_args_desc_t *filter_desc = arg_desc;
    uint8_t idx = (uint8_t)(*buf);
    filter_desc = filter_desc->next;
    esp_ae_eq_filter_para_t *para = (esp_ae_eq_filter_para_t *)(buf + filter_desc->offset);
    esp_ae_err_t ret = esp_ae_eq_get_filter_para(eq->eq_hd, idx, para);
    if (ret != ESP_AE_ERR_OK) {
        return ESP_GMF_ERR_FAIL;
    }
    return ESP_GMF_ERR_OK;
}

static esp_gmf_err_t __eq_enable_filter(esp_gmf_audio_element_handle_t handle, esp_gmf_args_desc_t *arg_desc,
                                        uint8_t *buf, int buf_len)
{
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, arg_desc, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, buf, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_gmf_eq_t *eq = (esp_gmf_eq_t *)handle;
    ESP_GMF_NULL_CHECK(TAG, eq->set_info, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_gmf_args_desc_t *filter_desc = arg_desc;
    uint8_t idx = (uint8_t)(*buf);
    filter_desc = filter_desc->next;
    uint8_t is_enable = (uint8_t)(*(buf + filter_desc->offset));
    eq->set_info[idx].is_filter_enabled = is_enable;
    return ESP_GMF_ERR_OK;
}

static esp_gmf_err_t esp_gmf_eq_new(void *cfg, esp_gmf_obj_handle_t *handle)
{
    *handle = NULL;
    esp_ae_eq_cfg_t *eq_cfg = (esp_ae_eq_cfg_t *)cfg;
    esp_gmf_obj_handle_t new_obj = NULL;
    esp_gmf_err_t ret = esp_gmf_eq_init(eq_cfg, &new_obj);
    if (ret != ESP_GMF_ERR_OK) {
        return ret;
    }
    ret = esp_gmf_eq_cast(eq_cfg, new_obj);
    if (ret != ESP_GMF_ERR_OK) {
        esp_gmf_obj_delete(new_obj);
        return ret;
    }
    *handle = (void *)new_obj;
    return ret;
}

static esp_gmf_job_err_t esp_gmf_eq_open(esp_gmf_audio_element_handle_t self, void *para)
{
    esp_gmf_eq_t *eq = (esp_gmf_eq_t *)self;
    esp_ae_eq_cfg_t *eq_info = (esp_ae_eq_cfg_t *)OBJ_GET_CFG(self);
    ESP_GMF_NULL_CHECK(TAG, eq_info, {return ESP_GMF_JOB_ERR_FAIL;});
    eq->bytes_per_sample = (eq_info->bits_per_sample >> 3) * eq_info->channel;
    esp_ae_eq_open(eq_info, &eq->eq_hd);
    ESP_GMF_CHECK(TAG, eq->eq_hd, {return ESP_GMF_JOB_ERR_FAIL;}, "Failed to create eq handle");
    eq->filter_num = eq_info->filter_num;
    GMF_AUDIO_UPDATE_SND_INFO(self, eq_info->sample_rate, eq_info->bits_per_sample, eq_info->channel);
    eq->set_info = esp_gmf_oal_calloc(1, eq_info->filter_num * sizeof(esp_gmf_eq_set_info_t));
    ESP_GMF_MEM_VERIFY(TAG, eq->set_info, {return ESP_GMF_JOB_ERR_FAIL;},
                       "set information", eq_info->filter_num * sizeof(esp_gmf_eq_set_info_t));
    esp_ae_eq_filter_para_t *para_tmp = eq_info->para;
    for (int i = 0; i < eq_info->filter_num; i++) {
        memcpy(&(eq->set_info[i].para), para_tmp, sizeof(esp_ae_eq_filter_para_t));
        para_tmp++;
    }
    ESP_LOGD(TAG, "Open, %p", eq);
    return ESP_GMF_ERR_OK;
}

static esp_gmf_job_err_t esp_gmf_eq_process(esp_gmf_audio_element_handle_t self, void *para)
{
    esp_gmf_eq_t *eq = (esp_gmf_eq_t *)self;
    int out_len = -1;
    ESP_GMF_RET_ON_NOT_OK(TAG, eq_update_apply_setting(eq), {return ESP_GMF_JOB_ERR_FAIL;}, "Failed to apply eq setting");
    esp_gmf_port_handle_t in_port = ESP_GMF_ELEMENT_GET(self)->in;
    esp_gmf_port_handle_t out_port = ESP_GMF_ELEMENT_GET(self)->out;
    esp_gmf_payload_t *in_load = NULL;
    esp_gmf_payload_t *out_load = NULL;
    esp_gmf_err_io_t load_ret = esp_gmf_port_acquire_in(in_port, &in_load, ESP_GMF_ELEMENT_GET(eq)->in_attr.data_size, ESP_GMF_MAX_DELAY);
    ESP_GMF_PORT_ACQUIRE_IN_CHECK(TAG, load_ret, out_len, {goto __eq_release;});
    int samples_num = in_load->valid_size / (eq->bytes_per_sample);
    if (in_port->is_shared == 1) {
        out_load = in_load;
    }
    load_ret = esp_gmf_port_acquire_out(out_port, &out_load, samples_num ? samples_num * eq->bytes_per_sample : in_load->buf_length, ESP_GMF_MAX_DELAY);
    ESP_GMF_PORT_ACQUIRE_OUT_CHECK(TAG, load_ret, out_len, {goto __eq_release;});
    if (samples_num > 0) {
        esp_ae_err_t ret = esp_ae_eq_process(eq->eq_hd, samples_num, in_load->buf, out_load->buf);
        ESP_GMF_RET_ON_ERROR(TAG, ret, {out_len = ESP_GMF_JOB_ERR_FAIL; goto __eq_release;}, "Equalize process error %d", ret);
    }
    ESP_LOGV(TAG, "Samples: %d, IN-PLD: %p-%p-%d-%d-%d, OUT-PLD: %p-%p-%d-%d-%d",
             samples_num, in_load, in_load->buf, in_load->valid_size, in_load->buf_length, in_load->is_done,
             out_load, out_load->buf, out_load->valid_size, out_load->buf_length, out_load->is_done);
    out_load->valid_size = samples_num * eq->bytes_per_sample;
    out_load->is_done = in_load->is_done;
    out_len = out_load->valid_size;
    if (out_len > 0) {
        esp_gmf_audio_el_update_file_pos((esp_gmf_element_handle_t)self, out_len);
    }
    if (in_load->is_done) {
        out_len = ESP_GMF_JOB_ERR_DONE;
        ESP_LOGD(TAG, "Equalize done, out len: %d", out_load->valid_size);
    }
__eq_release:
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

static esp_gmf_job_err_t esp_gmf_eq_close(esp_gmf_audio_element_handle_t self, void *para)
{
    esp_gmf_eq_t *eq = (esp_gmf_eq_t *)self;
    ESP_LOGD(TAG, "Closed, %p", self);
    if (eq->eq_hd != NULL) {
        esp_ae_eq_close(eq->eq_hd);
        eq->eq_hd = NULL;
    }
    if (eq->set_info != NULL) {
        esp_gmf_oal_free(eq->set_info);
        eq->set_info = NULL;
    }
    return ESP_GMF_ERR_OK;
}

static esp_gmf_err_t eq_received_event_handler(esp_gmf_event_pkt_t *evt, void *ctx)
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
            eq_change_src_info(self, info.sample_rates, info.channels, info.bits);
            ESP_LOGD(TAG, "RECV element info, from: %s-%p, next: %p, self: %s-%p, type: %x, state: %s, rate: %d, ch: %d, bits: %d",
                     OBJ_GET_TAG(el), el, esp_gmf_node_for_next((esp_gmf_node_t *)el), OBJ_GET_TAG(self), self, evt->type,
                     esp_gmf_event_get_state_str(state), info.sample_rates, info.channels, info.bits);
            // Change the state to ESP_GMF_EVENT_STATE_INITIALIZED, then add to working list.
            esp_gmf_element_set_state(self, ESP_GMF_EVENT_STATE_INITIALIZED);
        }
    }
    return ESP_GMF_ERR_OK;
}

static esp_gmf_err_t esp_gmf_eq_destroy(esp_gmf_audio_element_handle_t self)
{
    esp_gmf_eq_t *eq = (esp_gmf_eq_t *)self;
    ESP_LOGD(TAG, "Destroyed, %p", self);
    free_esp_ae_eq_cfg(OBJ_GET_CFG(self));
    esp_gmf_audio_el_deinit(self);
    esp_gmf_oal_free(eq);
    return ESP_GMF_ERR_OK;
}

esp_gmf_err_t esp_gmf_eq_set_para(esp_gmf_audio_element_handle_t handle, uint8_t idx, esp_ae_eq_filter_para_t *para)
{
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_gmf_method_t *method_head = NULL;
    esp_gmf_method_t *method = NULL;
    esp_gmf_element_get_method((esp_gmf_element_handle_t)handle, &method_head);
    esp_gmf_method_found(method_head, ESP_GMF_METHOD_EQ_SET_PARA, &method);
    uint8_t buf[28] = {0};
    esp_gmf_args_set_value(method->args_desc, ESP_GMF_METHOD_EQ_SET_PARA_ARG_IDX, buf, (uint8_t *)&idx, sizeof(idx));
    memcpy(buf + sizeof(idx), para, sizeof(esp_ae_eq_filter_para_t));
    return esp_gmf_element_exe_method((esp_gmf_element_handle_t)handle, ESP_GMF_METHOD_EQ_SET_PARA, buf, sizeof(buf));
}

esp_gmf_err_t esp_gmf_eq_get_para(esp_gmf_audio_element_handle_t handle, uint8_t idx, esp_ae_eq_filter_para_t *para)
{
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_gmf_method_t *method_head = NULL;
    esp_gmf_method_t *method = NULL;
    esp_gmf_element_get_method((esp_gmf_element_handle_t)handle, &method_head);
    esp_gmf_method_found(method_head, ESP_GMF_METHOD_EQ_GET_PARA, &method);
    uint8_t buf[28] = {0};
    esp_gmf_args_set_value(method->args_desc, ESP_GMF_METHOD_EQ_GET_PARA_ARG_IDX, buf, (uint8_t *)&idx, sizeof(idx));
    esp_gmf_err_t ret = esp_gmf_element_exe_method((esp_gmf_element_handle_t)handle, ESP_GMF_METHOD_EQ_GET_PARA, buf, sizeof(buf));
    if (ret != ESP_GMF_ERR_OK) {
        return ESP_GMF_ERR_FAIL;
    }
    memcpy(para, buf + sizeof(idx), sizeof(esp_ae_eq_filter_para_t));
    return ret;
}

esp_gmf_err_t esp_gmf_eq_enable_filter(esp_gmf_audio_element_handle_t handle, uint8_t idx, bool is_enable)
{
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_gmf_method_t *method_head = NULL;
    esp_gmf_method_t *method = NULL;
    esp_gmf_element_get_method((esp_gmf_element_handle_t)handle, &method_head);
    esp_gmf_method_found(method_head, ESP_GMF_METHOD_EQ_ENABLE_FILTER, &method);
    uint8_t buf[4] = {0};
    esp_gmf_args_set_value(method->args_desc, ESP_GMF_METHOD_EQ_ENABLE_FILTER_ARG_IDX, buf, (uint8_t *)&idx, sizeof(idx));
    esp_gmf_args_set_value(method->args_desc, ESP_GMF_METHOD_EQ_ENABLE_FILTER_ARG_PARA, buf, (uint8_t *)&is_enable, sizeof(is_enable));
    return esp_gmf_element_exe_method((esp_gmf_element_handle_t)handle, ESP_GMF_METHOD_EQ_ENABLE_FILTER, buf, sizeof(buf));
}

esp_gmf_err_t esp_gmf_eq_init(esp_ae_eq_cfg_t *config, esp_gmf_obj_handle_t *handle)
{
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    *handle = NULL;
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
    esp_gmf_eq_t *eq = esp_gmf_oal_calloc(1, sizeof(esp_gmf_eq_t));
    ESP_GMF_MEM_VERIFY(TAG, eq, {return ESP_GMF_ERR_MEMORY_LACK;}, "eq", sizeof(esp_gmf_eq_t));
    esp_gmf_obj_t *obj = (esp_gmf_obj_t *)eq;
    obj->new_obj = esp_gmf_eq_new;
    obj->del_obj = esp_gmf_eq_destroy;
    esp_ae_eq_filter_para_t *tmp = NULL;
    if (config) {
        esp_ae_eq_cfg_t *new_config = NULL;
        if (config->para == NULL) {
            config->para = esp_gmf_default_eq_paras;
            config->filter_num = sizeof(esp_gmf_default_eq_paras) / sizeof(esp_ae_eq_filter_para_t);
        }
        dupl_esp_ae_eq_cfg(config, &new_config);
        ESP_GMF_CHECK(TAG, new_config, {ret = ESP_GMF_ERR_MEMORY_LACK; goto EQ_INI_FAIL;}, "Failed to allocate eq configuration");
        esp_gmf_obj_set_config(obj, new_config, sizeof(esp_ae_eq_cfg_t));
    }
    ret = esp_gmf_obj_set_tag(obj, "eq");
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, goto EQ_INI_FAIL, "Failed to set obj tag");
    esp_gmf_element_cfg_t el_cfg = {0};
    ESP_GMF_ELEMENT_IN_PORT_ATTR_SET(el_cfg.in_attr, ESP_GMF_EL_PORT_CAP_SINGLE, 0, 0,
        ESP_GMF_PORT_TYPE_BLOCK | ESP_GMF_PORT_TYPE_BYTE, ESP_GMF_ELEMENT_PORT_DATA_SIZE_DEFAULT);
    ESP_GMF_ELEMENT_IN_PORT_ATTR_SET(el_cfg.out_attr, ESP_GMF_EL_PORT_CAP_SINGLE, 0, 0,
        ESP_GMF_PORT_TYPE_BLOCK | ESP_GMF_PORT_TYPE_BYTE, ESP_GMF_ELEMENT_PORT_DATA_SIZE_DEFAULT);
    el_cfg.dependency = true;
    ret = esp_gmf_audio_el_init(eq, &el_cfg);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, goto EQ_INI_FAIL, "Failed to initialize eq element");
    *handle = obj;
    ESP_LOGD(TAG, "Initialization, %s-%p", OBJ_GET_TAG(obj), obj);
    if (tmp) {
        esp_gmf_oal_free(tmp);
    }
    return ESP_GMF_ERR_OK;
EQ_INI_FAIL:
    if (tmp) {
        esp_gmf_oal_free(tmp);
    }
    esp_gmf_obj_delete(obj);
    return ret;
}

esp_gmf_err_t esp_gmf_eq_cast(esp_ae_eq_cfg_t *config, esp_gmf_obj_handle_t handle)
{
    ESP_GMF_NULL_CHECK(TAG, config, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
    esp_ae_eq_cfg_t *cfg = NULL;
    dupl_esp_ae_eq_cfg(config, &cfg);
    ESP_GMF_CHECK(TAG, cfg, {return ESP_GMF_ERR_MEMORY_LACK;}, "Failed to duplicate eq configuration");
    // Free memory before overwriting
    free_esp_ae_eq_cfg(OBJ_GET_CFG(handle));
    esp_gmf_obj_set_config(handle, cfg, sizeof(*config));
    esp_gmf_audio_element_t *eq_el = (esp_gmf_audio_element_t *)handle;
    esp_gmf_args_desc_t *set_args = NULL;
    esp_gmf_args_desc_t *get_args = NULL;
    esp_gmf_args_desc_t *pointer_args = NULL;

    ret = esp_gmf_args_desc_append(&pointer_args, ESP_GMF_METHOD_EQ_SET_PARA_ARG_PARA_FT, ESP_GMF_ARGS_TYPE_UINT32,
                                   sizeof(uint32_t), offsetof(esp_ae_eq_filter_para_t, filter_type));
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, {return ret;}, "Failed to append argument");
    ret = esp_gmf_args_desc_append(&pointer_args, ESP_GMF_METHOD_EQ_SET_PARA_ARG_PARA_FC, ESP_GMF_ARGS_TYPE_UINT32,
                                   sizeof(uint32_t), offsetof(esp_ae_eq_filter_para_t, fc));
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, {return ret;}, "Failed to append argument");
    ret = esp_gmf_args_desc_append(&pointer_args, ESP_GMF_METHOD_EQ_SET_PARA_ARG_PARA_Q, ESP_GMF_ARGS_TYPE_FLOAT,
                                   sizeof(float), offsetof(esp_ae_eq_filter_para_t, q));
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, {return ret;}, "Failed to append argument");
    ret = esp_gmf_args_desc_append(&pointer_args, ESP_GMF_METHOD_EQ_SET_PARA_ARG_PARA_GAIN, ESP_GMF_ARGS_TYPE_FLOAT,
                                   sizeof(float), offsetof(esp_ae_eq_filter_para_t, gain));
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, {return ret;}, "Failed to append argument");
    ret = esp_gmf_args_desc_append(&set_args, ESP_GMF_METHOD_EQ_SET_PARA_ARG_IDX, ESP_GMF_ARGS_TYPE_UINT8, sizeof(uint8_t), 0);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, {return ret;}, "Failed to append argument");
    ret = esp_gmf_args_desc_append_array(&set_args, ESP_GMF_METHOD_EQ_SET_PARA_ARG_PARA, pointer_args,
                                         sizeof(esp_ae_eq_filter_para_t), sizeof(uint8_t));
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, {return ret;}, "Failed to append argument");
    ret = esp_gmf_element_register_method(eq_el, ESP_GMF_METHOD_EQ_SET_PARA, __eq_set_para, set_args);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, {return ret;}, "Failed to register method");

    ret = esp_gmf_args_desc_copy(set_args, &get_args);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, {return ret;}, "Failed to copy argument");
    ret = esp_gmf_element_register_method(eq_el, ESP_GMF_METHOD_EQ_GET_PARA, __eq_get_para, get_args);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, {return ret;}, "Failed to register method");

    set_args = NULL;
    get_args = NULL;
    ret = esp_gmf_args_desc_append(&set_args, ESP_GMF_METHOD_EQ_ENABLE_FILTER_ARG_IDX, ESP_GMF_ARGS_TYPE_UINT8, sizeof(uint8_t), 0);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, {return ret;}, "Failed to append argument");
    ret = esp_gmf_args_desc_append(&set_args, ESP_GMF_METHOD_EQ_ENABLE_FILTER_ARG_PARA, ESP_GMF_ARGS_TYPE_UINT8,
                                   sizeof(uint8_t), sizeof(uint8_t));
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, {return ret;}, "Failed to append argument");
    ret = esp_gmf_element_register_method(eq_el, ESP_GMF_METHOD_EQ_ENABLE_FILTER, __eq_enable_filter, set_args);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, {return ret;}, "Failed to register method");

    eq_el->base.ops.open = esp_gmf_eq_open;
    eq_el->base.ops.process = esp_gmf_eq_process;
    eq_el->base.ops.close = esp_gmf_eq_close;
    eq_el->base.ops.event_receiver = eq_received_event_handler;
    return ESP_GMF_ERR_OK;
}
