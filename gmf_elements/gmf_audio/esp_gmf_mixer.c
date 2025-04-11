/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <string.h>
#include "esp_log.h"
#include "esp_gmf_oal_mem.h"
#include "esp_gmf_audio_element.h"
#include "esp_gmf_node.h"
#include "esp_gmf_mixer.h"
#include "esp_heap_caps.h"
#include "gmf_audio_common.h"
#include "esp_gmf_audio_method_def.h"
#include "esp_gmf_cap.h"
#include "esp_gmf_caps_def.h"

#define MIXER_DEFAULT_PROC_TIME_MS (10)

/**
 * @brief Audio mixer context in GMF
 */
typedef struct
{
    bool   is_changed; /*!< The flag of whether mixer mode is changed */
    int8_t mode;       /*!< The mode of mixer */
} esp_gmf_mixer_set_info_t;

typedef struct {
    esp_gmf_audio_element_t   parent;           /*!< The GMF mixer handle */
    esp_ae_mixer_handle_t     mixer_hd;         /*!< The audio effects mixer handle */
    uint8_t                   bytes_per_sample; /*!< Bytes number of per sampling point */
    uint32_t                  process_num;      /*!< The data number of mixer processing */
    esp_gmf_payload_t       **in_load;          /*!< The array of input payload */
    esp_gmf_payload_t        *out_load;         /*!< The output payload */
    uint8_t                 **in_arr;           /*!< The input buffer pointer array of mixer */
    esp_gmf_mixer_set_info_t *set_info;         /*!< Changed information of mixer */
    uint8_t                   src_num;          /*!< The number of input stream source */
} esp_gmf_mixer_t;

static const char *TAG = "ESP_GMF_MIXER";

const esp_ae_mixer_info_t esp_gmf_default_mixer_src_info[] = {
    {1.0, 0.5, 500},
    {0.5, 0.0, 500},
};

static inline esp_gmf_err_t dupl_esp_ae_mixer_cfg(esp_ae_mixer_cfg_t *config, esp_ae_mixer_cfg_t **new_config)
{
    void *sub_cfg = NULL;
    *new_config = esp_gmf_oal_calloc(1, sizeof(*config));
    ESP_GMF_MEM_VERIFY(TAG, *new_config, {return ESP_GMF_ERR_MEMORY_LACK;}, "mixer configuration", sizeof(*config));
    memcpy(*new_config, config, sizeof(*config));
    if (config->src_info && (config->src_num > 0)) {
        sub_cfg = esp_gmf_oal_calloc(1, config->src_num * sizeof(esp_ae_mixer_info_t));
        ESP_GMF_MEM_VERIFY(TAG, sub_cfg, {esp_gmf_oal_free(*new_config); return ESP_GMF_ERR_MEMORY_LACK;},
                           "source information", config->src_num * sizeof(esp_ae_mixer_info_t));
        memcpy(sub_cfg, config->src_info, config->src_num * sizeof(esp_ae_mixer_info_t));
        (*new_config)->src_info = sub_cfg;
    }
    return ESP_GMF_JOB_ERR_OK;
}

static inline void free_esp_ae_mixer_cfg(esp_ae_mixer_cfg_t *config)
{
    if (config) {
        if (config->src_info && (config->src_info != esp_gmf_default_mixer_src_info)) {
            esp_gmf_oal_free(config->src_info);
        }
        esp_gmf_oal_free(config);
    }
}

static inline esp_gmf_job_err_t mixer_update_apply_setting(esp_gmf_mixer_t *mixer)
{
    esp_ae_err_t ret = 0;
    for (int8_t i = 0; i < mixer->src_num; i++) {
        if (mixer->set_info[i].is_changed == true) {
            ret = esp_ae_mixer_set_mode(mixer->mixer_hd, i, mixer->set_info[i].mode);
            if (ret != ESP_AE_ERR_OK) {
                return ESP_GMF_JOB_ERR_FAIL;
            }
            mixer->set_info[i].is_changed = false;
        }
    }
    return ESP_GMF_JOB_ERR_OK;
}

static inline void mixer_change_src_info(esp_gmf_audio_element_handle_t self, uint32_t src_rate, uint8_t src_ch, uint8_t src_bits)
{
    esp_ae_mixer_cfg_t *mixer_info = (esp_ae_mixer_cfg_t *)OBJ_GET_CFG(self);
    mixer_info->channel = src_ch;
    mixer_info->sample_rate = src_rate;
    mixer_info->bits_per_sample = src_bits;
}

static esp_gmf_err_t __mixer_set_mode(esp_gmf_audio_element_handle_t handle, esp_gmf_args_desc_t *arg_desc,
                                      uint8_t *buf, int buf_len)
{
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, arg_desc, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, buf, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_gmf_mixer_t *mixer = (esp_gmf_mixer_t *)handle;
    ESP_GMF_NULL_CHECK(TAG, mixer->set_info, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_gmf_args_desc_t *mix_desc = arg_desc;
    esp_ae_mixer_mode_t mode = {0};
    uint8_t src_idx = (uint8_t)(*buf);
    mix_desc = mix_desc->next;
    memcpy(&mode, buf + mix_desc->offset, mix_desc->size);
    mixer->set_info[src_idx].is_changed = true;
    mixer->set_info[src_idx].mode = mode;
    return ESP_GMF_ERR_OK;
}

static esp_gmf_err_t __mixer_set_audio_info(esp_gmf_audio_element_handle_t handle, esp_gmf_args_desc_t *arg_desc,
                                            uint8_t *buf, int buf_len)
{
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, arg_desc, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, buf, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_gmf_args_desc_t *mix_desc = arg_desc;
    uint32_t rate = *((uint32_t *)buf);
    mix_desc = mix_desc->next;
    uint8_t ch = (uint8_t)(*(buf + mix_desc->offset));
    mix_desc = mix_desc->next;
    uint8_t bits = (uint8_t)(*(buf + mix_desc->offset));
    esp_gmf_event_state_t state = -1;
    esp_gmf_element_get_state(handle, &state);
    if (state < ESP_GMF_EVENT_STATE_OPENING) {
        esp_ae_mixer_cfg_t *mixer_info = (esp_ae_mixer_cfg_t *)OBJ_GET_CFG(handle);
        ESP_GMF_NULL_CHECK(TAG, mixer_info, {return ESP_GMF_ERR_FAIL;});
        mixer_info->sample_rate = rate;
        mixer_info->bits_per_sample = bits;
        mixer_info->channel = ch;
    } else {
        ESP_LOGE(TAG, "Failed to set audio information due to invalid state: %s", esp_gmf_event_get_state_str(state));
        return ESP_GMF_ERR_FAIL;
    }
    return ESP_GMF_ERR_OK;
}

static esp_gmf_err_t esp_gmf_mixer_new(void *cfg, esp_gmf_obj_handle_t *handle)
{
    *handle = NULL;
    esp_ae_mixer_cfg_t *mixer_cfg = (esp_ae_mixer_cfg_t *)cfg;
    esp_gmf_obj_handle_t new_obj = NULL;
    esp_gmf_err_t ret = esp_gmf_mixer_init(mixer_cfg, &new_obj);
    if (ret != ESP_GMF_ERR_OK) {
        return ret;
    }
    ret = esp_gmf_mixer_cast(mixer_cfg, new_obj);
    if (ret != ESP_GMF_ERR_OK) {
        esp_gmf_obj_delete(new_obj);
        return ret;
    }
    *handle = (void *)new_obj;
    return ret;
}

static esp_gmf_job_err_t esp_gmf_mixer_open(esp_gmf_audio_element_handle_t self, void *para)
{
    esp_gmf_mixer_t *mixer = (esp_gmf_mixer_t *)self;
    esp_ae_mixer_cfg_t *mixer_info = (esp_ae_mixer_cfg_t *)OBJ_GET_CFG(self);
    ESP_GMF_NULL_CHECK(TAG, mixer_info, {return ESP_GMF_JOB_ERR_FAIL;})
    mixer->bytes_per_sample = (mixer_info->bits_per_sample >> 3) * mixer_info->channel;
    mixer->src_num = mixer_info->src_num;
    mixer->process_num = MIXER_DEFAULT_PROC_TIME_MS * mixer_info->sample_rate * mixer->bytes_per_sample / 1000;
    esp_ae_mixer_open(mixer_info, &mixer->mixer_hd);
    ESP_GMF_CHECK(TAG, mixer->mixer_hd, {return ESP_GMF_JOB_ERR_FAIL;}, "Failed to create mixer handle");
    mixer->in_load = esp_gmf_oal_calloc(1, sizeof(esp_gmf_payload_t *) * mixer_info->src_num);
    ESP_GMF_MEM_VERIFY(TAG, mixer->in_load, {return ESP_GMF_JOB_ERR_FAIL;},
                       "in load", sizeof(esp_gmf_payload_t *) * mixer_info->src_num);
    mixer->in_arr = esp_gmf_oal_calloc(1, sizeof(int *) * mixer_info->src_num);
    ESP_GMF_MEM_VERIFY(TAG, mixer->in_arr, {return ESP_GMF_JOB_ERR_FAIL;},
                       "in buffer array", sizeof(int *) * mixer_info->src_num);
    GMF_AUDIO_UPDATE_SND_INFO(self, mixer_info->sample_rate, mixer_info->bits_per_sample, mixer_info->channel);
    mixer->set_info = esp_gmf_oal_calloc(1, mixer->src_num * sizeof(esp_gmf_mixer_set_info_t));
    ESP_GMF_MEM_VERIFY(TAG, mixer->set_info, {return ESP_GMF_JOB_ERR_FAIL;},
                       "set information", mixer->src_num * sizeof(esp_gmf_mixer_set_info_t));
    ESP_LOGD(TAG, "Open, %p", self);
    return ESP_GMF_JOB_ERR_OK;
}

static esp_gmf_job_err_t esp_gmf_mixer_process(esp_gmf_audio_element_handle_t self, void *para)
{
    esp_gmf_mixer_t *mixer = (esp_gmf_mixer_t *)self;
    int out_len = -1;
    int read_len = 0;
    esp_gmf_err_io_t ret = ESP_GMF_IO_OK;
    int status_end = 0;
    ret = mixer_update_apply_setting(mixer);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, {return ESP_GMF_JOB_ERR_FAIL;}, "Failed to apply mixer setting");
    esp_gmf_port_handle_t in = ESP_GMF_ELEMENT_GET(self)->in;
    esp_gmf_port_handle_t in_port = in;
    esp_gmf_port_handle_t out_port = ESP_GMF_ELEMENT_GET(self)->out;
    esp_ae_mixer_cfg_t *mixer_info = (esp_ae_mixer_cfg_t *)OBJ_GET_CFG(self);
    int index = mixer_info->src_num;
    memset(mixer->in_load, 0, sizeof(esp_gmf_payload_t *) * index);
    mixer->out_load = NULL;
    int i = 0;
    int wait_time = 0;
    while (in_port != NULL) {
        wait_time = i == 0 ? ESP_GMF_MAX_DELAY : 0;
        ret = esp_gmf_port_acquire_in(in_port, &(mixer->in_load[i]), mixer->process_num, wait_time);
        if (ret == ESP_GMF_IO_FAIL) {
            ESP_LOGE(TAG, "Acquire in failed, idx:%d, ret: %d", i, ret);
            out_len = ESP_GMF_JOB_ERR_FAIL;
            goto __mixer_release;
        }
        if (ret == ESP_GMF_IO_TIMEOUT || ret == ESP_GMF_IO_ABORT || mixer->in_load[i]->is_done) {
            status_end++;
        }
        read_len = mixer->in_load[i]->valid_size;
        mixer->in_arr[i] = mixer->in_load[i]->buf;
        if (read_len < mixer->process_num) {
            memset(mixer->in_arr[i] + read_len, 0, mixer->process_num - read_len);
        }
        in_port = in_port->next;
        i++;
        ESP_LOGV(TAG, "IN: idx: %d load: %p, buf: %p, valid size: %d, buf length: %d, done: %d",
                 i, mixer->in_load[i], mixer->in_load[i]->buf, mixer->in_load[i]->valid_size,
                 mixer->in_load[i]->buf_length, mixer->in_load[i]->is_done);
    }
    // Down-mixer never stop in gmf, only user can set to stop
    if (status_end == index) {
        out_len = ESP_GMF_JOB_ERR_OK;
        goto __mixer_release;
    }
    ret = esp_gmf_port_acquire_out(out_port, &mixer->out_load, mixer->process_num, ESP_GMF_MAX_DELAY);
    ESP_GMF_PORT_ACQUIRE_OUT_CHECK(TAG, ret, out_len, {goto __mixer_release;});
    esp_ae_err_t porc_ret = esp_ae_mixer_process(mixer->mixer_hd, mixer->process_num / mixer->bytes_per_sample,
                                                 (void *)mixer->in_arr, mixer->out_load->buf);
    if (porc_ret != ESP_AE_ERR_OK) {
        ESP_LOGE(TAG, "Mix process error %d.", porc_ret);
        return ESP_GMF_JOB_ERR_FAIL;
    }
    ESP_LOGV(TAG, "OUT: load: %p, buf: %p, valid size: %d, buf length: %d",
             mixer->out_load, mixer->out_load->buf, mixer->out_load->valid_size, mixer->out_load->buf_length);
    mixer->out_load->valid_size = mixer->process_num;
    out_len = mixer->out_load->valid_size;
    if (out_len > 0) {
        esp_gmf_audio_el_update_file_pos((esp_gmf_element_handle_t)self, out_len);
    }
__mixer_release:
    in_port = in;
    i = 0;
    while (in_port != NULL && mixer->in_load[i] != NULL) {
        ret = esp_gmf_port_release_in(in_port, mixer->in_load[i], ESP_GMF_MAX_DELAY);
        ESP_GMF_PORT_RELEASE_IN_CHECK(TAG, ret, out_len, NULL);
        in_port = in_port->next;
        i++;
    }
    if (mixer->out_load != NULL) {
        ret = esp_gmf_port_release_out(out_port, mixer->out_load, ESP_GMF_MAX_DELAY);
        ESP_GMF_PORT_RELEASE_OUT_CHECK(TAG, ret, out_len, NULL);
    }
    return out_len;
}

static esp_gmf_job_err_t esp_gmf_mixer_close(esp_gmf_audio_element_handle_t self, void *para)
{
    esp_gmf_mixer_t *mixer = (esp_gmf_mixer_t *)self;
    ESP_LOGD(TAG, "Closed, %p", self);
    if (mixer->mixer_hd != NULL) {
        esp_ae_mixer_close(mixer->mixer_hd);
        mixer->mixer_hd = NULL;
    }
    if (mixer->in_arr != NULL) {
        esp_gmf_oal_free(mixer->in_arr);
        mixer->in_arr = NULL;
    }
    if (mixer->in_load != NULL) {
        esp_gmf_oal_free(mixer->in_load);
        mixer->in_load = NULL;
    }
    if (mixer->set_info != NULL) {
        esp_gmf_oal_free(mixer->set_info);
        mixer->set_info = NULL;
    }
    return ESP_GMF_ERR_OK;
}

static esp_gmf_err_t mixer_received_event_handler(esp_gmf_event_pkt_t *evt, void *ctx)
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
            mixer_change_src_info(self, info.sample_rates, info.channels, info.bits);
            ESP_LOGD(TAG, "RECV element info, from: %s-%p, next: %p, self: %s-%p, type: %x, state: %s, rate: %d, ch: %d, bits: %d",
                     OBJ_GET_TAG(el), el, esp_gmf_node_for_next((esp_gmf_node_t *)el), OBJ_GET_TAG(self), self, evt->type,
                     esp_gmf_event_get_state_str(state), info.sample_rates, info.channels, info.bits);
            // Change the state to ESP_GMF_EVENT_STATE_INITIALIZED, then add to working list.
            esp_gmf_element_set_state(self, ESP_GMF_EVENT_STATE_INITIALIZED);
        }
    }
    return ESP_GMF_ERR_OK;
}

static esp_gmf_err_t esp_gmf_mixer_destroy(esp_gmf_audio_element_handle_t self)
{
    esp_gmf_mixer_t *mixer = (esp_gmf_mixer_t *)self;
    ESP_LOGD(TAG, "Destroyed, %p", self);
    free_esp_ae_mixer_cfg(OBJ_GET_CFG(self));
    esp_gmf_audio_el_deinit(self);
    esp_gmf_oal_free(mixer);
    return ESP_GMF_ERR_OK;
}

static esp_gmf_err_t _load_mixer_caps_func(esp_gmf_cap_t **caps)
{
    ESP_GMF_MEM_CHECK(TAG, caps, return ESP_ERR_INVALID_ARG);
    esp_gmf_cap_t dec_caps = {0};
    dec_caps.cap_eightcc = ESP_GMF_CAPS_AUDIO_MIXER;
    dec_caps.attr_fun = NULL;
    int ret = esp_gmf_cap_append(caps, &dec_caps);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, {return ret;}, "Failed to create capability");
    return ESP_GMF_ERR_OK;
}

static esp_gmf_err_t _load_mixer_methods_func(esp_gmf_method_t **method)
{
    ESP_GMF_MEM_CHECK(TAG, method, return ESP_ERR_INVALID_ARG);
    esp_gmf_args_desc_t *set_args = NULL;
    esp_gmf_err_t ret = esp_gmf_args_desc_append(&set_args, ESP_GMF_METHOD_MIXER_SET_INFO_ARG_RATE, ESP_GMF_ARGS_TYPE_UINT32, sizeof(uint32_t), 0);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, {return ret;}, "Failed to append RATE argument");
    ret = esp_gmf_args_desc_append(&set_args, ESP_GMF_METHOD_MIXER_SET_INFO_ARG_CH, ESP_GMF_ARGS_TYPE_UINT8,
                                   sizeof(uint8_t), sizeof(uint32_t));
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, {return ret;}, "Failed to append CHANNEL argument");
    ret = esp_gmf_args_desc_append(&set_args, ESP_GMF_METHOD_MIXER_SET_INFO_ARG_BITS, ESP_GMF_ARGS_TYPE_UINT8,
                                   sizeof(uint8_t), sizeof(uint8_t) + sizeof(uint32_t));
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, {return ret;}, "Failed to append BITS argument");
    ret = esp_gmf_method_append(method, ESP_GMF_METHOD_MIXER_SET_INFO, __mixer_set_audio_info, set_args);
    ESP_GMF_RET_ON_ERROR(TAG, ret, {return ret;}, "Failed to register %s method", ESP_GMF_METHOD_MIXER_SET_INFO);

    set_args = NULL;
    ret = esp_gmf_args_desc_append(&set_args, ESP_GMF_METHOD_MIXER_SET_MODE_ARG_IDX, ESP_GMF_ARGS_TYPE_UINT8, sizeof(uint8_t), 0);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, {return ret;}, "Failed to append INDEX argument");
    ret = esp_gmf_args_desc_append(&set_args, ESP_GMF_METHOD_MIXER_SET_MODE_ARG_MODE, ESP_GMF_ARGS_TYPE_INT32,
                                   sizeof(int32_t), sizeof(uint8_t));
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, {return ret;}, "Failed to append MODE argument");
    ret = esp_gmf_method_append(method, ESP_GMF_METHOD_MIXER_SET_MODE, __mixer_set_mode, set_args);
    ESP_GMF_RET_ON_ERROR(TAG, ret, {return ret;}, "Failed to register %s method", ESP_GMF_METHOD_MIXER_SET_MODE);
    return ESP_GMF_ERR_OK;
}

esp_gmf_err_t esp_gmf_mixer_set_mode(esp_gmf_audio_element_handle_t handle, uint8_t src_idx, esp_ae_mixer_mode_t mode)
{
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_gmf_method_t *method_head = NULL;
    esp_gmf_method_t *method = NULL;
    esp_gmf_element_get_method((esp_gmf_element_handle_t)handle, &method_head);
    esp_gmf_method_found(method_head, ESP_GMF_METHOD_MIXER_SET_MODE, &method);
    uint8_t buf[5] = {0};
    esp_gmf_args_set_value(method->args_desc, ESP_GMF_METHOD_MIXER_SET_MODE_ARG_IDX, buf, (uint8_t *)&src_idx, sizeof(src_idx));
    esp_gmf_args_set_value(method->args_desc, ESP_GMF_METHOD_MIXER_SET_MODE_ARG_MODE, buf, (uint8_t *)&mode, sizeof(mode));
    return esp_gmf_element_exe_method((esp_gmf_element_handle_t)handle, ESP_GMF_METHOD_MIXER_SET_MODE, buf, sizeof(buf));
}

esp_gmf_err_t esp_gmf_mixer_set_audio_info(esp_gmf_audio_element_handle_t handle, uint32_t sample_rate,
                                           uint8_t bits, uint8_t channel)
{
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_gmf_method_t *method_head = NULL;
    esp_gmf_method_t *method = NULL;
    esp_gmf_element_get_method((esp_gmf_element_handle_t)handle, &method_head);
    esp_gmf_method_found(method_head, ESP_GMF_METHOD_MIXER_SET_INFO, &method);
    uint8_t buf[6] = {0};
    esp_gmf_args_set_value(method->args_desc, ESP_GMF_METHOD_MIXER_SET_INFO_ARG_RATE, buf, (uint8_t *)&sample_rate, sizeof(sample_rate));
    esp_gmf_args_set_value(method->args_desc, ESP_GMF_METHOD_MIXER_SET_INFO_ARG_CH, buf, (uint8_t *)&channel, sizeof(channel));
    esp_gmf_args_set_value(method->args_desc, ESP_GMF_METHOD_MIXER_SET_INFO_ARG_BITS, buf, (uint8_t *)&bits, sizeof(bits));
    return esp_gmf_element_exe_method((esp_gmf_element_handle_t)handle, ESP_GMF_METHOD_MIXER_SET_INFO, buf, sizeof(buf));
}

esp_gmf_err_t esp_gmf_mixer_init(esp_ae_mixer_cfg_t *config, esp_gmf_obj_handle_t *handle)
{
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    *handle = NULL;
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
    esp_gmf_mixer_t *mixer = esp_gmf_oal_calloc(1, sizeof(esp_gmf_mixer_t));
    ESP_GMF_MEM_VERIFY(TAG, mixer, {return ESP_GMF_ERR_MEMORY_LACK;}, "mixer", sizeof(esp_gmf_mixer_t));
    esp_gmf_obj_t *obj = (esp_gmf_obj_t *)mixer;
    obj->new_obj = esp_gmf_mixer_new;
    obj->del_obj = esp_gmf_mixer_destroy;
    if (config) {
        if (config->src_info == NULL) {
            config->src_info = (esp_ae_mixer_info_t*)esp_gmf_default_mixer_src_info;
            config->src_num = sizeof(esp_gmf_default_mixer_src_info) / sizeof(esp_ae_mixer_info_t);
        }
        esp_ae_mixer_cfg_t *new_config = NULL;
        dupl_esp_ae_mixer_cfg(config, &new_config);
        ESP_GMF_CHECK(TAG, new_config, {ret = ESP_GMF_ERR_MEMORY_LACK; goto MIXER_INIT_FAIL;}, "Failed to allocate mixer configuration");
        esp_gmf_obj_set_config(obj, new_config, sizeof(esp_ae_mixer_cfg_t));
    }
    ret = esp_gmf_obj_set_tag(obj, "mixer");
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, goto MIXER_INIT_FAIL, "Failed to set obj tag");
    esp_gmf_element_cfg_t el_cfg = {0};
    ESP_GMF_ELEMENT_IN_PORT_ATTR_SET(el_cfg.in_attr, ESP_GMF_EL_PORT_CAP_MULTI, 0, 0,
        ESP_GMF_PORT_TYPE_BLOCK | ESP_GMF_PORT_TYPE_BYTE, ESP_GMF_ELEMENT_PORT_DATA_SIZE_DEFAULT);
    ESP_GMF_ELEMENT_IN_PORT_ATTR_SET(el_cfg.out_attr, ESP_GMF_EL_PORT_CAP_SINGLE, 0, 0,
        ESP_GMF_PORT_TYPE_BLOCK | ESP_GMF_PORT_TYPE_BYTE, ESP_GMF_ELEMENT_PORT_DATA_SIZE_DEFAULT);
    el_cfg.dependency = true;
    ret = esp_gmf_audio_el_init(mixer, &el_cfg);
    ESP_GMF_RET_ON_NOT_OK(TAG, ret, goto MIXER_INIT_FAIL, "Failed to initialize mixer element");
    *handle = obj;
    ESP_LOGD(TAG, "Initialization, %s-%p", OBJ_GET_TAG(obj), obj);
    return ESP_GMF_ERR_OK;
MIXER_INIT_FAIL:
    esp_gmf_obj_delete(obj);
    return ret;
}

esp_gmf_err_t esp_gmf_mixer_cast(esp_ae_mixer_cfg_t *config, esp_gmf_obj_handle_t handle)
{
    ESP_GMF_NULL_CHECK(TAG, config, {return ESP_GMF_ERR_INVALID_ARG;});
    ESP_GMF_NULL_CHECK(TAG, handle, {return ESP_GMF_ERR_INVALID_ARG;});
    esp_ae_mixer_cfg_t *cfg = NULL;
    dupl_esp_ae_mixer_cfg(config, &cfg);
    ESP_GMF_CHECK(TAG, cfg, {return ESP_GMF_ERR_MEMORY_LACK;}, "Failed to duplicate mixer configuration");
    // Free memory before overwriting
    free_esp_ae_mixer_cfg(OBJ_GET_CFG(handle));
    esp_gmf_obj_set_config(handle, cfg, sizeof(*config));
    esp_gmf_audio_element_t *mixer_el = (esp_gmf_audio_element_t *)handle;

    mixer_el->base.ops.open = esp_gmf_mixer_open;
    mixer_el->base.ops.process = esp_gmf_mixer_process;
    mixer_el->base.ops.close = esp_gmf_mixer_close;
    mixer_el->base.ops.event_receiver = mixer_received_event_handler;
    mixer_el->base.ops.load_caps = _load_mixer_caps_func;
    mixer_el->base.ops.load_methods = _load_mixer_methods_func;
    return ESP_GMF_ERR_OK;
}
