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
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "sys/queue.h"

#include "esp_gmf_oal_mutex.h"
#include "esp_gmf_oal_thread.h"
#include "esp_gmf_oal_mem.h"
#include "esp_gmf_node.h"
#include "esp_gmf_task.h"
#include "esp_log.h"

static const char *TAG = "ESP_GMF_TASK";

#define DEFAULT_TASK_OPT_MAX_TIME_MS (2000 / portTICK_PERIOD_MS)

static inline esp_gmf_err_t esp_gmf_event_state_notify(esp_gmf_task_handle_t handle, esp_gmf_event_type_t type, esp_gmf_event_state_t st)
{
    esp_gmf_task_t *tsk = (esp_gmf_task_t *)handle;
    esp_gmf_event_pkt_t evt = {
        .from = tsk,
        .type = type,
        .sub = st,
        .payload = NULL,
        .payload_size = 0,
    };
    if (tsk->event_func == NULL) {
        return ESP_GMF_ERR_OK;
    }
    return tsk->event_func(&evt, tsk->ctx);
}

static inline esp_gmf_err_t esp_gmf_task_event_state_change_and_notify(esp_gmf_task_handle_t handle, esp_gmf_event_state_t new_st)
{
    int ret = ESP_GMF_ERR_OK;
    esp_gmf_task_t *tsk = (esp_gmf_task_t *)handle;
    if (tsk->state != new_st) {
        // Notification first then change the state to keep the previous state in callback function
        ret = esp_gmf_event_state_notify(tsk, ESP_GMF_EVT_TYPE_CHANGE_STATE, new_st);
        if (ret == ESP_GMF_ERR_OK) {
            tsk->state = new_st;
        }
    }
    return ret;
}

static inline esp_gmf_err_t esp_gmf_task_event_loading_job(esp_gmf_task_handle_t handle, esp_gmf_event_state_t new_st)
{
    int ret = ESP_GMF_ERR_OK;
    esp_gmf_task_t *tsk = (esp_gmf_task_t *)handle;
    if (tsk->state != new_st) {
        ret = esp_gmf_event_state_notify(tsk, ESP_GMF_EVT_TYPE_LOADING_JOB, new_st);
        if (ret == ESP_GMF_ERR_OK) {
            tsk->state = new_st;
        }
    }
    return ret;
}

static inline int esp_gmf_task_acquire_singal(esp_gmf_task_handle_t handle, int ticks)
{
    esp_gmf_task_t *tsk = (esp_gmf_task_t *)handle;
    if (xSemaphoreTake(tsk->wait_sem, ticks) != pdPASS) {
        return ESP_GMF_ERR_FAIL;
    }
    return ESP_GMF_ERR_OK;
}

static inline int esp_gmf_task_release_singal(esp_gmf_task_handle_t handle, int ticks)
{
    esp_gmf_task_t *tsk = (esp_gmf_task_t *)handle;
    if (xSemaphoreGive(tsk->wait_sem) != pdTRUE) {
        return ESP_GMF_ERR_FAIL;
    }
    return ESP_GMF_ERR_OK;
}

static int get_jobs_num(esp_gmf_job_t *job)
{
    int k = 1;
    esp_gmf_job_t *tmp = job;
    while (tmp && (tmp = tmp->next)) {
        k++;
    }
    return k;
}

static inline void __esp_gmf_task_free(esp_gmf_task_handle_t handle)
{
    esp_gmf_task_t *tsk = (esp_gmf_task_t *)handle;
    if (tsk->lock) {
        esp_gmf_oal_mutex_destroy(tsk->lock);
    }
    if (tsk->block_sem) {
        vSemaphoreDelete(tsk->block_sem);
    }
    if (tsk->wait_sem) {
        vSemaphoreDelete(tsk->wait_sem);
    }
    if (tsk->api_sync_sem) {
        vSemaphoreDelete(tsk->api_sync_sem);
    }
    esp_gmf_oal_free(tsk);
}

int esp_gmf_job_item_free(void *ptr)
{
    esp_gmf_job_t *job = (esp_gmf_job_t *)ptr;
    esp_gmf_oal_free((void *)job->label);
    esp_gmf_oal_free(job);
    return ESP_GMF_ERR_OK;
}

static inline void __esp_gmf_task_delete_jobs(esp_gmf_task_handle_t handle)
{
    esp_gmf_task_t *tsk = (esp_gmf_task_t *)handle;
    esp_gmf_node_clear((esp_gmf_node_t **)&tsk->working, esp_gmf_job_item_free);
}

static inline int process_func(esp_gmf_task_handle_t handle, void *para)
{
    esp_gmf_task_t *tsk = (esp_gmf_task_t *)handle;
    esp_gmf_job_t *worker = tsk->working;
    if ((worker == NULL) || (worker->func == NULL)) {
        ESP_LOGE(TAG, "Jobs list are invalid[%p, %p]", tsk, worker);
        return ESP_GMF_ERR_INVALID_ARG;
    }
    int result = ESP_GMF_ERR_OK;
    uint8_t is_stop = 0;
    while (worker && worker->func) {
        ESP_LOGD(TAG, "Running, job:%p, ctx:%p", worker->func, worker->ctx);
        worker->ret = worker->func(worker->ctx, NULL);
        ESP_LOGV(TAG, "Job ret:%d, [tsk:%s-%p:%p-%p-%s]", worker->ret, OBJ_GET_TAG((esp_gmf_obj_handle_t)tsk), tsk, worker, worker->ctx, worker->label);
        if (worker->ret == ESP_GMF_JOB_ERR_CONTINUE) {
            // The means need more loops
            worker = tsk->working;
            continue;
        } else if (worker->ret == ESP_GMF_JOB_ERR_DONE) {
            ESP_LOGI(TAG, "Job is done, [tsk:%s-%p, wk:%p, job:%p-%s]", OBJ_GET_TAG((esp_gmf_obj_handle_t)tsk), tsk, worker, worker->ctx, worker->label);
            esp_gmf_job_t *tmp = worker->next;
            esp_gmf_node_del_at((esp_gmf_node_t **)&tsk->working, (esp_gmf_node_t *)worker);
            esp_gmf_job_item_free(worker);
            worker = tmp;
            if (worker == NULL) {
                ESP_LOGD(TAG, "All jobs are finished, [tsk:%s-%p]", OBJ_GET_TAG((esp_gmf_obj_handle_t)tsk), tsk);
                esp_gmf_task_event_loading_job(tsk, ESP_GMF_EVENT_STATE_FINISHED);
                worker = tsk->working;
                if (worker == NULL) {
                    ESP_LOGV(TAG, "No more jobs after finished, [%s-%p, new job:%p]", OBJ_GET_TAG((esp_gmf_obj_handle_t)tsk), tsk, worker);
                    continue;
                }
                ESP_LOGD(TAG, "After finished, [%s-%p, wk:%p, new job:%p-%s]", OBJ_GET_TAG((esp_gmf_obj_handle_t)tsk), tsk, worker, worker->ctx, worker->label);
            }
            continue;
        } else if (worker->ret == ESP_GMF_JOB_ERR_FAIL) {
            ESP_LOGE(TAG, "Job failed[tsk:%s-%p:%p-%p-%s], ret:%d, st:%s", OBJ_GET_TAG((esp_gmf_obj_handle_t)tsk), tsk, worker, worker->ctx, worker->label, worker->ret,
                     esp_gmf_event_get_state_str(tsk->state));
            if (tsk->state != ESP_GMF_EVENT_STATE_STOPPED) {
                __esp_gmf_task_delete_jobs(tsk);
                esp_gmf_task_event_loading_job(tsk, ESP_GMF_EVENT_STATE_ERROR);
                is_stop = 1;
                worker = tsk->working;
                if (worker == NULL) {
                    ESP_LOGV(TAG, "No more jobs after failed, [%s-%p, new job:%p]", OBJ_GET_TAG((esp_gmf_obj_handle_t)tsk), tsk, worker);
                    continue;
                }
                ESP_LOGD(TAG, "After failed, [%s-%p, wk:%p,new job:%p-%s]", OBJ_GET_TAG((esp_gmf_obj_handle_t)tsk), tsk, worker, worker->ctx, worker->label);
                continue;
            }
        }
        if (tsk->_pause) {
            ESP_LOGI(TAG, "Pause job, [%s-%p, wk:%p, job:%p-%s],st:%s", OBJ_GET_TAG((esp_gmf_obj_handle_t)tsk), tsk, worker, worker->ctx, worker->label,
                     esp_gmf_event_get_state_str(tsk->state));
            if (tsk->state != ESP_GMF_EVENT_STATE_ERROR) {
                esp_gmf_task_event_state_change_and_notify(tsk, ESP_GMF_EVENT_STATE_PAUSED);
                xSemaphoreGive(tsk->api_sync_sem);

                esp_gmf_task_acquire_singal(tsk, portMAX_DELAY);
                ESP_LOGI(TAG, "Resume job, [%s-%p, wk:%p, job:%p-%s]", OBJ_GET_TAG((esp_gmf_obj_handle_t)tsk), tsk, worker, worker->ctx, worker->label);
                esp_gmf_task_event_state_change_and_notify(tsk, ESP_GMF_EVENT_STATE_RUNNING);
            }
            tsk->_pause = 0;
            xSemaphoreGive(tsk->api_sync_sem);
        }
        if (tsk->_stop && (tsk->state != ESP_GMF_EVENT_STATE_ERROR)) {
            ESP_LOGV(TAG, "Stop job, [%s-%p, wk:%p, job:%p-%s]", OBJ_GET_TAG((esp_gmf_obj_handle_t)tsk), tsk, worker, worker->ctx, worker->label);
            __esp_gmf_task_delete_jobs(tsk);
            esp_gmf_task_event_loading_job(tsk, ESP_GMF_EVENT_STATE_STOPPED);
            worker = tsk->working;
            tsk->_stop = 0;
            is_stop = 1;
            if (worker == NULL) {
                ESP_LOGV(TAG, "No more jobs after stopped, [%s-%p, new job:%p]", OBJ_GET_TAG((esp_gmf_obj_handle_t)tsk), tsk, worker);
                continue;
            }
            ESP_LOGD(TAG, "After stopped, [%s-%p, new job:%p-%p-%s]", OBJ_GET_TAG((esp_gmf_obj_handle_t)tsk), tsk, worker, worker->ctx, worker->label);
            continue;
        }

        ESP_LOGD(TAG, "Find next job to process, [%s-%p, cur:%p-%p-%s]", OBJ_GET_TAG((esp_gmf_obj_handle_t)tsk), tsk, worker, worker->ctx, worker->label);

        esp_gmf_job_t *tmp = worker->next;
        if (worker->times == ESP_GMF_JOB_TIMES_ONCE) {
            ESP_LOGI(TAG, "One times job is complete, del[wk:%p,ctx:%p, label:%s]", worker, worker->ctx, worker->label);
            esp_gmf_node_del_at((esp_gmf_node_t **)&tsk->working, (esp_gmf_node_t *)worker);
            esp_gmf_job_item_free(worker);
            worker = NULL;
        }
        worker = tmp;
        if (tmp == NULL && tsk->working) {
            worker = tsk->working;
        }
        ESP_LOGD(TAG, "Found next job[%p] to process", worker);
    }
    ESP_LOGV(TAG, "Worker exit, [%p-%s], st:%s, stop:%s", tsk, OBJ_GET_TAG((esp_gmf_obj_handle_t)tsk), esp_gmf_event_get_state_str(tsk->state), is_stop == 0 ? "NO" : "YES");
    esp_gmf_event_state_notify(tsk, ESP_GMF_EVT_TYPE_CHANGE_STATE, tsk->state);
    if (is_stop) {
        is_stop = 0;
        xSemaphoreGive(tsk->api_sync_sem);
    }
    return result;
}

static void esp_gmf_thread_fun(void *pv)
{
    esp_gmf_task_t *tsk = (esp_gmf_task_t *)pv;
    tsk->_destroy = 0;
    while (tsk->_task_run) {
        while ((tsk->working == NULL) || (tsk->_running == 0)) {
            ESP_LOGI(TAG, "Waiting to run... [tsk:%s-%p, wk:%p, run:%d]", OBJ_GET_TAG((esp_gmf_obj_handle_t)tsk), tsk, tsk->working, tsk->_running);
            xSemaphoreTake(tsk->block_sem, portMAX_DELAY);
            if (tsk->_destroy) {
                tsk->_destroy = 0;
                ESP_LOGD(TAG, "Thread will be destroyed, [%s,%p]", OBJ_GET_TAG((esp_gmf_obj_handle_t)tsk), tsk);
                goto ESP_GMF_THREAD_EXIT;
            }
        }
        int ret = esp_gmf_task_event_state_change_and_notify(tsk, ESP_GMF_EVENT_STATE_RUNNING);
        xSemaphoreGive(tsk->api_sync_sem);
        if (ret != ESP_GMF_ERR_OK) {
            tsk->_running = 0;
            ESP_LOGE(TAG, "Failed on prepare, [%s,%p],ret:%d", OBJ_GET_TAG((esp_gmf_obj_handle_t)tsk), tsk, ret);
            continue;
        }
        // Loop jobs until done or error
        process_func(tsk, tsk->ctx);
        tsk->_running = 0;
    }
ESP_GMF_THREAD_EXIT:
    tsk->state = ESP_GMF_EVENT_STATE_NONE;
    xSemaphoreGive(tsk->api_sync_sem);
    ESP_LOGD(TAG, "Thread destroyed! [%s,%p]", OBJ_GET_TAG((esp_gmf_obj_handle_t)tsk), tsk);
    esp_gmf_oal_thread_delete(tsk->oal_thread);
}

esp_gmf_err_t esp_gmf_task_init(void *config, esp_gmf_task_handle_t *tsk_hd)
{
    ESP_GMF_NULL_CHECK(TAG, tsk_hd, return ESP_GMF_ERR_INVALID_ARG);
    ESP_GMF_NULL_CHECK(TAG, config, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_task_t *handle = calloc(1, sizeof(struct _esp_gmf_task));
    ESP_GMF_MEM_CHECK(TAG, handle, return ESP_GMF_ERR_MEMORY_LACK);
    handle->lock = esp_gmf_oal_mutex_create();
    ESP_GMF_MEM_CHECK(TAG, handle->lock, goto _el_init_failed);
    handle->block_sem = xSemaphoreCreateBinary();
    ESP_GMF_MEM_CHECK(TAG, handle->block_sem, goto _el_init_failed);
    handle->wait_sem = xSemaphoreCreateBinary();
    ESP_GMF_MEM_CHECK(TAG, handle->wait_sem, goto _el_init_failed);
    handle->api_sync_sem = xSemaphoreCreateBinary();
    ESP_GMF_MEM_CHECK(TAG, handle->api_sync_sem, goto _el_init_failed);
    esp_gmf_task_cfg_t *cfg = (esp_gmf_task_cfg_t *)config;
    handle->event_func = cfg->cb;
    handle->ctx = cfg->ctx;
    handle->api_sync_time = DEFAULT_TASK_OPT_MAX_TIME_MS;

    char tag[ESP_GMF_TAG_MAX_LEN] = {0};
    if (cfg->name) {
        snprintf(tag, ESP_GMF_TAG_MAX_LEN, "TSK_%s", cfg->name);
    } else {
        snprintf(tag, ESP_GMF_TAG_MAX_LEN, "TSK_%p", handle);
    }

    esp_gmf_obj_t *obj = (esp_gmf_obj_t *)handle;
    int ret = esp_gmf_obj_set_config(obj, cfg, sizeof(*cfg));
    ESP_GMF_RET_ON_ERROR(TAG, ret, goto _el_init_failed, "Failed set OBJ configuration");
    ret = esp_gmf_obj_set_tag(obj, tag);
    ESP_GMF_RET_ON_ERROR(TAG, ret, goto _el_init_failed, "Failed set OBJ tag");
    obj->new = esp_gmf_task_init;
    obj->delete = esp_gmf_task_deinit;

    if (cfg->thread.stack > 0) {
        handle->thread.stack = cfg->thread.stack;
        handle->thread.stack_in_ext = cfg->thread.stack_in_ext;
    }
    if (cfg->thread.prio) {
        handle->thread.prio = cfg->thread.prio;
    } else {
        handle->thread.prio = DEFAULT_ESP_GMF_TASK_PRIO;
    }
    if (cfg->thread.core) {
        handle->thread.core = cfg->thread.core;
    } else {
        handle->thread.core = DEFAULT_ESP_GMF_TASK_CORE;
    }
    handle->_task_run = true;
    if (handle->thread.stack > 0) {
        ret = esp_gmf_oal_thread_create(&handle->oal_thread, OBJ_GET_TAG(obj), esp_gmf_thread_fun, handle, handle->thread.stack,
                                        handle->thread.prio, handle->thread.stack_in_ext, handle->thread.core);
        if (ret == ESP_GMF_ERR_FAIL) {
            handle->_task_run = false;
            ESP_LOGE(TAG, "Create thread failed, [%s]", OBJ_GET_TAG((esp_gmf_obj_handle_t)handle));
            goto _el_init_failed;
        }
    }
    handle->state = ESP_GMF_EVENT_STATE_INITIALIZED;
    *tsk_hd = handle;
    return ESP_GMF_ERR_OK;
_el_init_failed:
    __esp_gmf_task_free(handle);
    return ESP_GMF_ERR_FAIL;
}

esp_gmf_err_t esp_gmf_task_deinit(esp_gmf_task_handle_t handle)
{
    ESP_GMF_NULL_CHECK(TAG, handle, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_task_t *tsk = (esp_gmf_task_t *)handle;
    esp_gmf_oal_mutex_lock(tsk->lock);
    xSemaphoreTake(tsk->api_sync_sem, 0);
    if ((tsk->state == ESP_GMF_EVENT_STATE_RUNNING)
        || (tsk->state == ESP_GMF_EVENT_STATE_PAUSED)) {
        tsk->_stop = 1;
    }
    if (tsk->state == ESP_GMF_EVENT_STATE_PAUSED) {
        esp_gmf_task_release_singal(tsk, portMAX_DELAY);
    }
    tsk->_task_run = 0;
    tsk->_destroy = 1;
    xSemaphoreGive(tsk->block_sem);
    xSemaphoreTake(tsk->api_sync_sem, portMAX_DELAY);
    ESP_LOGD(TAG, "%s, %s", __func__, OBJ_GET_TAG(tsk));
    __esp_gmf_task_delete_jobs(tsk);
    esp_gmf_oal_mutex_unlock(tsk->lock);
    esp_gmf_obj_set_config((esp_gmf_obj_handle_t)tsk, ((esp_gmf_obj_t *)tsk)->cfg, 0);
    esp_gmf_obj_set_tag((esp_gmf_obj_handle_t)tsk, NULL);
    __esp_gmf_task_free(tsk);
    return ESP_GMF_ERR_OK;
}

esp_gmf_err_t esp_gmf_task_register_ready_job(esp_gmf_task_handle_t handle, const char *label, esp_gmf_job_func job, esp_gmf_job_times_t times, void *ctx, bool done)
{
    ESP_GMF_NULL_CHECK(TAG, handle, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_task_t *tsk = (esp_gmf_task_t *)handle;
    esp_gmf_job_t *new_job = NULL;
    new_job = esp_gmf_oal_calloc(1, sizeof(esp_gmf_job_t));
    ESP_GMF_MEM_CHECK(TAG, new_job, return ESP_GMF_ERR_MEMORY_LACK;);
    new_job->label = esp_gmf_oal_strdup(label == NULL ? "NULL" : label);
    ESP_GMF_MEM_CHECK(TAG, new_job->label, { esp_gmf_oal_free(new_job); return ESP_GMF_ERR_MEMORY_LACK;});
    new_job->func = job;
    new_job->ctx = ctx;
    new_job->times = times;
    if (tsk->working == NULL) {
        tsk->working = new_job;
    } else {
        esp_gmf_node_add_last((esp_gmf_node_t *)tsk->working, (esp_gmf_node_t *)new_job);
    }
    ESP_LOGD(TAG, "Reg new job to task:%p, item:%p, label:%s, func:%p, ctx:%p cnt:%d", tsk, new_job, new_job->label, job, ctx, get_jobs_num(tsk->working));
    if (done) {
        xSemaphoreGive(tsk->block_sem);
    }
    return ESP_GMF_ERR_OK;
}

esp_gmf_err_t esp_gmf_task_set_event_func(esp_gmf_task_handle_t handle, esp_gmf_event_cb cb, void *ctx)
{
    ESP_GMF_NULL_CHECK(TAG, handle, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_task_t *tsk = (esp_gmf_task_t *)handle;
    esp_gmf_oal_mutex_lock(tsk->lock);
    tsk->event_func = cb;
    tsk->ctx = ctx;
    esp_gmf_oal_mutex_unlock(tsk->lock);
    return ESP_GMF_ERR_OK;
}

esp_gmf_err_t esp_gmf_task_run(esp_gmf_task_handle_t handle)
{
    ESP_GMF_NULL_CHECK(TAG, handle, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_task_t *tsk = (esp_gmf_task_t *)handle;
    esp_gmf_oal_mutex_lock(tsk->lock);
    ESP_LOGD(TAG, "%s, %s-%p,st:%s", __func__, OBJ_GET_TAG((esp_gmf_obj_handle_t)tsk), tsk, esp_gmf_event_get_state_str(tsk->state));
    if (tsk->state == ESP_GMF_EVENT_STATE_PAUSED || tsk->state == ESP_GMF_EVENT_STATE_RUNNING) {
        esp_gmf_oal_mutex_unlock(tsk->lock);
        ESP_LOGW(TAG, "Can't run on %s, [%s,%p]", esp_gmf_event_get_state_str(tsk->state), OBJ_GET_TAG((esp_gmf_obj_handle_t)tsk), tsk);
        return ESP_GMF_ERR_NOT_SUPPORT;
    }
    if (tsk->_task_run == false) {
        esp_gmf_oal_mutex_unlock(tsk->lock);
        ESP_LOGW(TAG, "No task for run, %s, [%s,%p]", esp_gmf_event_get_state_str(tsk->state), OBJ_GET_TAG((esp_gmf_obj_handle_t)tsk), tsk);
        return ESP_GMF_ERR_INVALID_STATE;
    }
    tsk->_running = 1;
    xSemaphoreGive(tsk->block_sem);
    if (xSemaphoreTake(tsk->api_sync_sem, tsk->api_sync_time) != pdPASS) {
        ESP_LOGE(TAG, "Run timeout,[%s,%p]", OBJ_GET_TAG((esp_gmf_obj_handle_t)tsk), tsk);
        esp_gmf_oal_mutex_unlock(tsk->lock);
        return ESP_GMF_ERR_TIMEOUT;
    }
    esp_gmf_oal_mutex_unlock(tsk->lock);
    return ESP_GMF_ERR_OK;
}

esp_gmf_err_t esp_gmf_task_stop(esp_gmf_task_handle_t handle)
{
    ESP_GMF_NULL_CHECK(TAG, handle, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_task_t *tsk = (esp_gmf_task_t *)handle;
    esp_gmf_oal_mutex_lock(tsk->lock);
    ESP_LOGD(TAG, "%s, %s-%p, st:%s", __func__, OBJ_GET_TAG((esp_gmf_obj_handle_t)tsk), tsk, esp_gmf_event_get_state_str(tsk->state));
    if ((tsk->state != ESP_GMF_EVENT_STATE_RUNNING)
        && (tsk->state != ESP_GMF_EVENT_STATE_PAUSED)) {
        esp_gmf_oal_mutex_unlock(tsk->lock);
        ESP_LOGW(TAG, "Already stopped, %s, [%s,%p]", esp_gmf_event_get_state_str(tsk->state), OBJ_GET_TAG((esp_gmf_obj_handle_t)tsk), tsk);
        return ESP_GMF_ERR_OK;
    }
    if ((tsk->_task_run == false)) {
        esp_gmf_oal_mutex_unlock(tsk->lock);
        ESP_LOGW(TAG, "The task is not running, %s, [%s,%p]", esp_gmf_event_get_state_str(tsk->state), OBJ_GET_TAG((esp_gmf_obj_handle_t)tsk), tsk);
        return ESP_GMF_ERR_INVALID_STATE;
    }
    if ((tsk->state == ESP_GMF_EVENT_STATE_NONE)) {
        esp_gmf_oal_mutex_unlock(tsk->lock);
        ESP_LOGW(TAG, "Can't stop on %s, [%s,%p]", esp_gmf_event_get_state_str(tsk->state), OBJ_GET_TAG((esp_gmf_obj_handle_t)tsk), tsk);
        return ESP_GMF_ERR_NOT_SUPPORT;
    }
    tsk->_stop = 1;
    if (tsk->state == ESP_GMF_EVENT_STATE_PAUSED) {
        esp_gmf_task_release_singal(tsk, portMAX_DELAY);
    }
    if (xSemaphoreTake(tsk->api_sync_sem, tsk->api_sync_time) != pdPASS) {
        ESP_LOGE(TAG, "Stop timeout,[%s,%p]", OBJ_GET_TAG((esp_gmf_obj_handle_t)tsk), tsk);
        esp_gmf_oal_mutex_unlock(tsk->lock);
        return ESP_GMF_ERR_TIMEOUT;
    }
    esp_gmf_oal_mutex_unlock(tsk->lock);
    return ESP_GMF_ERR_OK;
}

esp_gmf_err_t esp_gmf_task_pause(esp_gmf_task_handle_t handle)
{
    ESP_GMF_NULL_CHECK(TAG, handle, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_task_t *tsk = (esp_gmf_task_t *)handle;

    esp_gmf_oal_mutex_lock(tsk->lock);
    ESP_LOGD(TAG, "%s, task:%s-%p, st:%s", __func__, OBJ_GET_TAG((esp_gmf_obj_handle_t)tsk), tsk, esp_gmf_event_get_state_str(tsk->state));
    if ((tsk->state == ESP_GMF_EVENT_STATE_STOPPED)
        || (tsk->state == ESP_GMF_EVENT_STATE_PAUSED)
        || (tsk->state == ESP_GMF_EVENT_STATE_FINISHED)
        || (tsk->state == ESP_GMF_EVENT_STATE_ERROR)) {
        esp_gmf_oal_mutex_unlock(tsk->lock);
        ESP_LOGW(TAG, "Without pause on %s, [%s,%p]", esp_gmf_event_get_state_str(tsk->state), OBJ_GET_TAG((esp_gmf_obj_handle_t)tsk), tsk);
        return ESP_GMF_ERR_OK;
    }

    if ((tsk->state != ESP_GMF_EVENT_STATE_RUNNING)) {
        esp_gmf_oal_mutex_unlock(tsk->lock);
        ESP_LOGW(TAG, "Can't pause on %s, [%s,%p]", esp_gmf_event_get_state_str(tsk->state), OBJ_GET_TAG((esp_gmf_obj_handle_t)tsk), tsk);
        return ESP_GMF_ERR_NOT_SUPPORT;
    }
    tsk->_pause = 1;
    if (xSemaphoreTake(tsk->api_sync_sem, tsk->api_sync_time) != pdPASS) {
        ESP_LOGE(TAG, "Pause timeout,[%s,%p]", OBJ_GET_TAG((esp_gmf_obj_handle_t)tsk), tsk);
        esp_gmf_oal_mutex_unlock(tsk->lock);
        return ESP_GMF_ERR_TIMEOUT;
    }
    esp_gmf_oal_mutex_unlock(tsk->lock);
    return ESP_GMF_ERR_OK;
}

esp_gmf_err_t esp_gmf_task_resume(esp_gmf_task_handle_t handle)
{
    ESP_GMF_NULL_CHECK(TAG, handle, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_task_t *tsk = (esp_gmf_task_t *)handle;
    esp_gmf_oal_mutex_lock(tsk->lock);
    ESP_LOGD(TAG, "%s, task:%s-%p,st:%s", __func__, OBJ_GET_TAG((esp_gmf_obj_handle_t)tsk), tsk, esp_gmf_event_get_state_str(tsk->state));
    if ((tsk->state != ESP_GMF_EVENT_STATE_PAUSED)) {
        esp_gmf_oal_mutex_unlock(tsk->lock);
        ESP_LOGW(TAG, "Can't resume on %s, [%s,%p]", esp_gmf_event_get_state_str(tsk->state), OBJ_GET_TAG((esp_gmf_obj_handle_t)tsk), tsk);
        return ESP_GMF_ERR_NOT_SUPPORT;
    }
    tsk->_pause = 0;
    esp_gmf_task_release_singal(tsk, portMAX_DELAY);
    if (xSemaphoreTake(tsk->api_sync_sem, tsk->api_sync_time) != pdPASS) {
        ESP_LOGE(TAG, "Resume timeout,[%s,%p]", OBJ_GET_TAG((esp_gmf_obj_handle_t)tsk), tsk);
        esp_gmf_oal_mutex_unlock(tsk->lock);
        return ESP_GMF_ERR_TIMEOUT;
    }
    esp_gmf_oal_mutex_unlock(tsk->lock);
    return ESP_GMF_ERR_OK;
}

esp_gmf_err_t esp_gmf_task_reset(esp_gmf_task_handle_t handle)
{
    ESP_GMF_NULL_CHECK(TAG, handle, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_task_t *tsk = (esp_gmf_task_t *)handle;
    tsk->state = ESP_GMF_EVENT_STATE_INITIALIZED;
    return ESP_GMF_ERR_OK;
}

esp_gmf_err_t esp_gmf_task_set_timeout(esp_gmf_task_handle_t handle, int wait_ms)
{
    ESP_GMF_NULL_CHECK(TAG, handle, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_task_t *tsk = (esp_gmf_task_t *)handle;
    tsk->api_sync_time = wait_ms / portTICK_PERIOD_MS;
    return ESP_GMF_ERR_OK;
}

esp_gmf_err_t esp_gmf_task_get_state(esp_gmf_task_handle_t handle, esp_gmf_event_state_t *state)
{
    ESP_GMF_NULL_CHECK(TAG, handle, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_task_t *tsk = (esp_gmf_task_t *)handle;
    if (state) {
        *state = tsk->state;
        return ESP_GMF_ERR_OK;
    }
    return ESP_GMF_ERR_INVALID_ARG;
}
