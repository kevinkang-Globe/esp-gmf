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
#include "esp_gmf_port.h"
#include "esp_gmf_element.h"
#include "esp_gmf_node.h"

static const char *TAG = "ESP_GMF_PORT";

static inline esp_gmf_err_io_t esp_gmf_port_dec_ref(esp_gmf_port_handle_t port, esp_gmf_payload_t *load, int wait_ticks)
{
    if (load == NULL) {
        load = port->self_payload;
    }
    if (port->ref_count > 0) {
        port->ref_count--;
        if ((port->ref_count == 0) && port->ops.release) {
            return port->ops.release(port->ctx, load, wait_ticks);
        }
    }
    return ESP_GMF_ERR_OK;
}

esp_gmf_err_t esp_gmf_port_init(esp_gmf_port_config_t *cfg, esp_gmf_port_handle_t *out_result)
{
    ESP_GMF_NULL_CHECK(TAG, cfg, return ESP_GMF_ERR_INVALID_ARG);
    ESP_GMF_NULL_CHECK(TAG, out_result, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_port_t *port = esp_gmf_oal_calloc(1, sizeof(esp_gmf_port_t));
    ESP_GMF_MEM_CHECK(TAG, port, return ESP_GMF_ERR_MEMORY_LACK);
    port->type = cfg->type;
    port->user_buf_len = cfg->buf_length;
    port->dir = cfg->dir;
    memcpy(&port->ops, &cfg->ops, sizeof(port->ops));
    port->ctx = cfg->ctx;
    port->wait_ticks = cfg->wait_ticks;
    port->is_shared = 1; // Shared the payload for other port by default
    *out_result = (esp_gmf_port_handle_t)port;
    ESP_LOGD(TAG, "Create a port:%p, t:%d, dir:%d, sub:%p, len:%d", port,
             port->type, port->dir, port->ctx, port->user_buf_len);
    return ESP_GMF_ERR_OK;
}

esp_gmf_err_t esp_gmf_port_deinit(esp_gmf_port_handle_t handle)
{
    ESP_GMF_NULL_CHECK(TAG, handle, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_port_t *port = (esp_gmf_port_t *)handle;
    ESP_LOGD(TAG, "Delete a port:%p, t:%d, dir:%d, self_payload:%p, ptr:%p", port,
             port->type, port->dir, port->self_payload, port->payload);
    esp_gmf_payload_delete(port->self_payload);
    if (port->ops.del && (port->dir == ESP_GMF_PORT_DIR_OUT)) {
        port->ops.del(port->ctx);
        port->ops.del = NULL;
        port->ctx = NULL;
    }
    esp_gmf_oal_free(port);
    return ESP_GMF_ERR_OK;
}

esp_gmf_err_t esp_gmf_port_set_payload(esp_gmf_port_handle_t handle, esp_gmf_payload_t *load)
{
    esp_gmf_port_t *port = (esp_gmf_port_t *)handle;
    ESP_GMF_NULL_CHECK(TAG, port, return ESP_GMF_ERR_INVALID_ARG);
    ESP_LOGD(TAG, "Set payload, cur:%p, new load:%p-b:%p-l:%d, port:%p", port->payload, load,
             load->buf ? load->buf : NULL, load->buf_length ? load->buf_length : 0, port);
    if (port->self_payload) {
        esp_gmf_payload_delete(port->self_payload);
        port->self_payload = NULL;
    }
    port->self_payload = load;
    return ESP_GMF_ERR_OK;
}

esp_gmf_err_t esp_gmf_port_clean_payload_done(esp_gmf_port_handle_t handle)
{
    esp_gmf_port_t *port = (esp_gmf_port_t *)handle;
    ESP_GMF_NULL_CHECK(TAG, port, return ESP_GMF_ERR_INVALID_ARG);
    if (port->self_payload) {
        esp_gmf_payload_clean_done(port->self_payload);
    }
    return ESP_GMF_ERR_OK;
}

esp_gmf_err_t esp_gmf_port_enable_payload_share(esp_gmf_port_handle_t handle, bool enable)
{
    esp_gmf_port_t *port = (esp_gmf_port_t *)handle;
    ESP_GMF_NULL_CHECK(TAG, port, return ESP_GMF_ERR_INVALID_ARG);
    port->is_shared = enable == true ? 1 : 0;
    return ESP_GMF_ERR_OK;
}

esp_gmf_err_t esp_gmf_port_reset(esp_gmf_port_handle_t handle)
{
    esp_gmf_port_t *port = (esp_gmf_port_t *)handle;
    ESP_GMF_NULL_CHECK(TAG, port, return ESP_GMF_ERR_INVALID_ARG);
    port->payload = NULL;
    if (port->self_payload) {
        esp_gmf_payload_clean_done(port->self_payload);
        port->self_payload->valid_size = 0;
    }
    return ESP_GMF_ERR_OK;
}

esp_gmf_err_t esp_gmf_port_set_wait_ticks(esp_gmf_port_handle_t handle, int wait_ticks_ms)
{
    esp_gmf_port_t *port = (esp_gmf_port_t *)handle;
    ESP_GMF_NULL_CHECK(TAG, port, return ESP_GMF_ERR_INVALID_ARG);
    ESP_LOGD(TAG, "P:%p, change wait ticks from %d to %d", port, port->wait_ticks, wait_ticks_ms);
    port->wait_ticks = wait_ticks_ms;
    return ESP_GMF_ERR_OK;
}

esp_gmf_err_t esp_gmf_port_set_reader(esp_gmf_port_handle_t handle, void *reader)
{
    esp_gmf_port_t *port = (esp_gmf_port_t *)handle;
    ESP_GMF_NULL_CHECK(TAG, port, return ESP_GMF_ERR_INVALID_ARG);
    port->reader = reader;
    return ESP_GMF_ERR_OK;
}

esp_gmf_err_t esp_gmf_port_set_writer(esp_gmf_port_handle_t handle, void *writer)
{
    esp_gmf_port_t *port = (esp_gmf_port_t *)handle;
    ESP_GMF_NULL_CHECK(TAG, port, return ESP_GMF_ERR_INVALID_ARG);
    port->writer = writer;
    return ESP_GMF_ERR_OK;
}

esp_gmf_err_t esp_gmf_port_add_last(esp_gmf_port_handle_t head, esp_gmf_port_handle_t io_inst)
{
    ESP_GMF_NULL_CHECK(TAG, head, return ESP_GMF_ERR_INVALID_ARG);
    ESP_GMF_NULL_CHECK(TAG, io_inst, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_port_t *tmp = (esp_gmf_port_t *)head;
    while (tmp && tmp->next) {
        tmp = tmp->next;
    }
    tmp->next = io_inst;
    io_inst->next = NULL;
    return ESP_GMF_ERR_OK;
}

esp_gmf_err_t esp_gmf_port_del_at(esp_gmf_port_handle_t *head, esp_gmf_port_handle_t io_inst)
{
    ESP_GMF_NULL_CHECK(TAG, head, return ESP_GMF_ERR_INVALID_ARG);
    ESP_GMF_NULL_CHECK(TAG, io_inst, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_port_t *tmp = (esp_gmf_port_t *)*head;
    esp_gmf_port_t *prev = (esp_gmf_port_t *)*head;
    while (tmp) {
        if (io_inst == tmp) {
            if (*head == tmp) {
                *head = tmp->next;
            } else {
                prev->next = tmp->next;
            }
            return ESP_GMF_ERR_OK;
        }
        prev = tmp;
        tmp = tmp->next;
    }
    return ESP_GMF_ERR_FAIL;
}

esp_gmf_err_io_t esp_gmf_port_acquire_in(esp_gmf_port_handle_t handle, esp_gmf_payload_t **load, uint32_t wanted_size, int wait_ticks)
{
    esp_gmf_port_t *port = (esp_gmf_port_t *)handle;
    ESP_GMF_NULL_CHECK(TAG, port, return ESP_GMF_IO_FAIL);
    ESP_GMF_NULL_CHECK(TAG, load, return ESP_GMF_IO_FAIL);
    if (port->dir != ESP_GMF_PORT_DIR_IN) {
        ESP_LOGE(TAG, "Wrong port direction! %s, p:%p-dir:%d", __func__, port, port->dir);
        return ESP_GMF_IO_FAIL;
    }
    int ret = ESP_GMF_ERR_OK;
    esp_gmf_element_handle_t el = (esp_gmf_element_handle_t)port->reader;
    if (el && port->writer) {
        // Not first element
        ESP_LOGD(TAG, "ACQ IN, GET, port:%p-%d, el:%p-%s, PLD[h:%p, b:%p, v:%d]",
                 port, port->type, el, OBJ_GET_TAG(el), port->payload, port->payload ? port->payload->buf : NULL, port->payload ? port->payload->valid_size : 0);
        if (port->payload) {
            *load = port->payload;
            ret = port->payload->valid_size;
            if (ESP_GMF_ELEMENT_GET(((esp_gmf_node_t *)el)->next)) {
                if ((port->payload->needs_free) && (port->is_shared) && ESP_GMF_ELEMENT_GET(((esp_gmf_node_t *)el)->next)->out) {
                    ESP_GMF_ELEMENT_GET(((esp_gmf_node_t *)el)->next)->out->payload = port->payload;
                }
            }
        } else {
            ESP_LOGE(TAG, "ACQ IN, there is no payload, p:%p, el:%p-%s", port, el, OBJ_GET_TAG(el));
            ret = ESP_GMF_IO_FAIL;
        }
    } else {
        if (*load == NULL) {
            if (port->self_payload == NULL) {
                esp_gmf_payload_new(&port->self_payload);
                ESP_GMF_MEM_CHECK(TAG, port->self_payload, return ESP_GMF_IO_FAIL);
                ESP_LOGI(TAG, "ACQ IN, new self payload:%p, port:%p, el:%p-%s", port->self_payload, port, el, OBJ_GET_TAG(el));
            }
            port->payload = port->self_payload;
            *load = port->self_payload;
        } else {
            port->payload = *load;
        }
        if (port->type == ESP_GMF_PORT_TYPE_BYTE) {
            // Check whether the buffer length is sufficient for use; if not, reallocate it.
            ret = esp_gmf_payload_realloc_buf(*load, wanted_size);
            ESP_GMF_RET_ON_ERROR(TAG, ret, return ESP_GMF_IO_FAIL, "ACQ IN, reallocate payload buffer failed, ret:%d, %s, p:%p, new_sz:%ld",
                                 ret, __func__, port, wanted_size);
            ret = (*load)->buf_length;
        }
        ESP_LOGD(TAG, "ACQ IN, port:%p-%d, el:%p-%s, PLD[p:%p, h:%p, b:%p, l:%d], nxt_el:%p-%s", port, port->type, el, OBJ_GET_TAG(el), port->payload,
                 *load, (*load)->buf, (*load)->buf_length, ESP_GMF_ELEMENT_GET(((esp_gmf_node_t *)el)->next), OBJ_GET_TAG(ESP_GMF_ELEMENT_GET(((esp_gmf_node_t *)el)->next)));
        if ((port->payload->needs_free) && (port->type != ESP_GMF_PORT_TYPE_BLOCK) && (port->is_shared)
            && ESP_GMF_ELEMENT_GET(((esp_gmf_node_t *)el)->next) && ESP_GMF_ELEMENT_GET(((esp_gmf_node_t *)el)->next)->out) {
            ESP_GMF_ELEMENT_GET(((esp_gmf_node_t *)el)->next)->out->payload = port->payload;
        }
        ret = port->ops.acquire(port->ctx, *load, wanted_size, wait_ticks);
        if (ret > 0) {
            port->ref_count = 1;
        }
    }
    return ret;
}

esp_gmf_err_io_t esp_gmf_port_release_in(esp_gmf_port_handle_t handle, esp_gmf_payload_t *load, int wait_ticks)
{
    esp_gmf_port_t *port = (esp_gmf_port_t *)handle;
    ESP_GMF_NULL_CHECK(TAG, port, return ESP_GMF_IO_FAIL);
    ESP_GMF_NULL_CHECK(TAG, load, return ESP_GMF_IO_FAIL);

    if (port->dir != ESP_GMF_PORT_DIR_IN) {
        ESP_LOGE(TAG, "Wrong port direction! %s, p:%p, pld:%p, buf_len:%d", __func__, port,
                 port->payload, port->user_buf_len);
        return ESP_GMF_IO_FAIL;
    }
    int ret = ESP_GMF_ERR_OK;
    esp_gmf_element_handle_t el = (esp_gmf_element_handle_t)port->reader;
    ESP_LOGD(TAG, "%s, p:%p, el:%s, PLD[p:%p, h:%p, b:%p, l:%d]", __func__, port, OBJ_GET_TAG(el), port->payload, load, load->buf, load->buf_length);
    if (el && port->writer) {
        if (port->ref_port) {
            ret = esp_gmf_port_dec_ref(port->ref_port, load, wait_ticks);
        }
        if (port->payload && port->is_shared) {
            port->payload = NULL;
        }
    } else {
        ret = esp_gmf_port_dec_ref(port, load, wait_ticks);
    }
    return ret;
}

esp_gmf_err_io_t esp_gmf_port_acquire_aligned_out(esp_gmf_port_handle_t handle, esp_gmf_payload_t **load, uint8_t align,
        uint32_t wanted_size, int wait_ticks)
{
    esp_gmf_port_t *port = (esp_gmf_port_t *)handle;
    ESP_GMF_NULL_CHECK(TAG, port, return ESP_GMF_IO_FAIL);
    ESP_GMF_NULL_CHECK(TAG, load, return ESP_GMF_IO_FAIL);
    if (port->dir != ESP_GMF_PORT_DIR_OUT) {
        ESP_LOGE(TAG, "Wrong port direction! %s, p:%p, pld:%p, buf_len:%d, want:%ld", __func__, port,
                 port->payload, port->user_buf_len, wanted_size);
        return ESP_GMF_IO_FAIL;
    }
    int ret = ESP_GMF_ERR_OK;
    esp_gmf_element_handle_t el = (esp_gmf_element_handle_t)port->writer;
    if ((*load) && ((*load) == ESP_GMF_ELEMENT_GET(el)->in->payload)) {
        if (wanted_size > ESP_GMF_ELEMENT_GET(el)->in->payload->buf_length) {
            ESP_LOGE(TAG, "Input and output use the same payload, but the acquired length is too large. I:%p-%d, O:%p-%ld",
                     ESP_GMF_ELEMENT_GET(el)->in->payload, ESP_GMF_ELEMENT_GET(el)->in->payload->buf_length, (*load), wanted_size);
            return ESP_GMF_IO_FAIL;
        }
        // When in and out use same payload, clear the next element out payload which is set by acquire in.
        if (ESP_GMF_ELEMENT_GET(((esp_gmf_node_t *)el)->next) && ESP_GMF_ELEMENT_GET(((esp_gmf_node_t *)el)->next)->out) {
            ESP_GMF_ELEMENT_GET(((esp_gmf_node_t *)el)->next)->out->payload = NULL;
        }
    }
    if (el && port->reader) {
        ESP_LOGD(TAG, "ACQ OUT, SET, port:%p-%d, el:%p-%s, PLD[in:%p, self:%p, nxt:%p]", port, port->type, el,
                 OBJ_GET_TAG(el), *load, port->self_payload, ESP_GMF_ELEMENT_GET(((esp_gmf_node_t *)el)->next)->in->payload);
        if (*load) {
            ret = esp_gmf_payload_realloc_aligned_buf(*load, align, wanted_size);
            ESP_GMF_RET_ON_ERROR(TAG, ret, return ESP_GMF_IO_FAIL, "ACQ OUT, SET NEXT, reallocate payload buffer failed, el:%s, p:%p, sz:%d, new_sz:%ld",
                                 OBJ_GET_TAG(el), port, port->user_buf_len, wanted_size);
            ret = (*load)->buf_length;
            if (ESP_GMF_ELEMENT_GET(((esp_gmf_node_t *)el)->next)->in->payload) {
                ESP_LOGD(TAG, "ACQ OUT, COPY DATA TO NEXT[%p], port:%p-%d, el:%p-%s",
                         ESP_GMF_ELEMENT_GET(((esp_gmf_node_t *)el)->next)->in->payload, port, port->type, el, OBJ_GET_TAG(el));
                esp_gmf_payload_copy_data(ESP_GMF_ELEMENT_GET(((esp_gmf_node_t *)el)->next)->in->payload, *load);
            } else {
                esp_gmf_port_t *next_in = ESP_GMF_ELEMENT_GET(((esp_gmf_node_t *)el)->next)->in;
                next_in->payload = *load;
                esp_gmf_port_t *ref_in = ESP_GMF_ELEMENT_GET(el)->in;
                ref_in = ref_in->ref_port ? ref_in->ref_port : ref_in;
                next_in->ref_port = ref_in;
                ref_in->ref_count++;
            }
            ESP_LOGD(TAG, "ACQ OUT, SET NEXT, port:%p-%d, el:%p-%s, PLD[in:%p-done:%d, nxt:%p]", port, port->type, el,
                     OBJ_GET_TAG(el), *load, (*load)->is_done, ESP_GMF_ELEMENT_GET(((esp_gmf_node_t *)el)->next)->in->payload);
        } else {
            if (port->payload) {
                *load = port->payload;
            } else {
                if (port->self_payload == NULL) {
                    esp_gmf_payload_new(&port->self_payload);
                    ESP_GMF_MEM_CHECK(TAG, port->self_payload, return ESP_GMF_IO_FAIL);
                    ESP_LOGI(TAG, "ACQ OUT SET, new self payload:%p, p:%p, el:%p-%s", port->self_payload, port, el, OBJ_GET_TAG(el));
                }
                port->payload = port->self_payload;
                *load = port->self_payload;
            }
            // Check whether the buffer length is sufficient for use; if not, reallocate it.
            ret = esp_gmf_payload_realloc_aligned_buf(*load, align, wanted_size);
            ESP_GMF_RET_ON_ERROR(TAG, ret, return ESP_GMF_IO_FAIL, "ACQ OUT, SET, reallocate payload buffer failed, el:%s, p:%p, sz:%d, new_sz:%ld",
                                 OBJ_GET_TAG(el), port, port->user_buf_len, wanted_size);
            ret = (*load)->buf_length;
            ESP_GMF_ELEMENT_GET(((esp_gmf_node_t *)el)->next)->in->payload = *load;
        }
    } else {
        ESP_LOGD(TAG, "ACQ OUT, self:%p, payload:%p,load:%p, port:%p, el:%p-%s", port->self_payload, port->payload, *load, port, el, OBJ_GET_TAG(el));
        if (*load == NULL) {
            if (port->payload) {
                *load = port->payload;
            } else {
                if (port->self_payload == NULL) {
                    esp_gmf_payload_new(&port->self_payload);
                    ESP_GMF_MEM_CHECK(TAG, port->self_payload, return ESP_GMF_IO_FAIL);
                    ESP_LOGI(TAG, "ACQ OUT, new self payload:%p, port:%p, el:%p-%s", port->self_payload, port, el, OBJ_GET_TAG(el));
                }
                port->payload = port->self_payload;
                *load = port->self_payload;
            }
        } else {
            port->payload = *load;
        }
        if (port->type == ESP_GMF_PORT_TYPE_BYTE) {
            // Check whether the buffer length is sufficient for use; if not, reallocate it.
            ret = esp_gmf_payload_realloc_aligned_buf(*load, align, wanted_size);
            ESP_GMF_RET_ON_ERROR(TAG, ret, return ESP_GMF_IO_FAIL, "ACQ OUT, reallocate payload buffer failed, el:%s, p:%p, ld:%p, sz:%d, new_sz:%ld",
                                 OBJ_GET_TAG(el), port, *load, port->user_buf_len, wanted_size);
            ret = (*load)->buf_length;
        }
        ESP_LOGD(TAG, "ACQ OUT, port:%p-%d, el:%p-%s, PLD[p:%p, h:%p, b:%p, v:%d, l:%d]", port, port->type, el, OBJ_GET_TAG(el),
                 port->payload, *load, (*load)->buf, (*load)->valid_size, (*load)->buf_length);
        if (ESP_GMF_ELEMENT_GET(((esp_gmf_node_t *)el)->next)) {
            if ((port->payload->needs_free) && ESP_GMF_ELEMENT_GET(((esp_gmf_node_t *)el)->next)->in) {
                ESP_GMF_ELEMENT_GET(((esp_gmf_node_t *)el)->next)->in->payload = port->payload;
            }
        }
        ret = port->ops.acquire(port->ctx, *load, wanted_size, wait_ticks);
    }
    return ret;
}

esp_gmf_err_io_t esp_gmf_port_acquire_out(esp_gmf_port_handle_t handle, esp_gmf_payload_t **load, uint32_t wanted_size, int wait_ticks)
{
    return esp_gmf_port_acquire_aligned_out(handle, load, 0, wanted_size, wait_ticks);
}

esp_gmf_err_io_t esp_gmf_port_release_out(esp_gmf_port_handle_t handle, esp_gmf_payload_t *load, int wait_ticks)
{
    esp_gmf_port_t *port = (esp_gmf_port_t *)handle;
    ESP_GMF_NULL_CHECK(TAG, port, return ESP_GMF_IO_FAIL);
    ESP_GMF_NULL_CHECK(TAG, load, return ESP_GMF_IO_FAIL);
    if (port->dir != ESP_GMF_PORT_DIR_OUT) {
        ESP_LOGE(TAG, "Wrong port direction! %s, p:%p, pld:%p, buf_len:%d", __func__, port,
                 port->payload, port->user_buf_len);
        return ESP_GMF_IO_FAIL;
    }
    esp_gmf_element_handle_t el = (esp_gmf_element_handle_t)port->writer;
    int ret = ESP_GMF_ERR_OK;
    ESP_LOGD(TAG, "%s, p:%p, el:%s,reader:%p, PLD[h:%p, b:%p, l:%d]", __func__, port, OBJ_GET_TAG(el), port->reader, load, load->buf, load->buf_length);
    if (el && port->reader) {
        port->payload = NULL;
    } else {
        ret = port->ops.release(port->ctx, load, wait_ticks);
    }
    return ret;
}
