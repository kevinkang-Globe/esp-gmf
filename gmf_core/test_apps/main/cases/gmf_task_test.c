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

#include "unity.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "esp_gmf_oal_mem.h"
#include "esp_gmf_task.h"

static const char *TAG = "TEST_ESP_GMF_TASK";

static esp_gmf_job_err_t prepare1_return;
static esp_gmf_job_err_t prepare2_return;
static esp_gmf_job_err_t prepare3_return;
static esp_gmf_job_err_t working1_return;
static esp_gmf_job_err_t working2_return;
static esp_gmf_job_err_t working3_return;
static esp_gmf_job_err_t cleanup1_return;
static esp_gmf_job_err_t cleanup2_return;
static esp_gmf_job_err_t cleanup3_return;

esp_gmf_job_err_t prepare1(void *self, void *para)
{
    printf("%s,%p,%p\r\n", __func__, self, para);
    return prepare1_return;
}
esp_gmf_job_err_t prepare2(void *self, void *para)
{
    printf("%s,%p,%p\r\n", __func__, self, para);
    return prepare2_return;
}
esp_gmf_job_err_t prepare3(void *self, void *para)
{
    printf("%s,%p,%p\r\n", __func__, self, para);
    return prepare3_return;
}
esp_gmf_job_err_t working1(void *self, void *para)
{
    vTaskDelay(100 / portTICK_PERIOD_MS);
    printf("%s,%p,%p\r\n", __func__, self, para);
    return working1_return;
}
esp_gmf_job_err_t working2(void *self, void *para)
{
    vTaskDelay(200 / portTICK_PERIOD_MS);
    printf("%s,%p,%p\r\n", __func__, self, para);
    return working2_return;
}
esp_gmf_job_err_t working3(void *self, void *para)
{
    vTaskDelay(300 / portTICK_PERIOD_MS);
    printf("%s,%p,%p\r\n", __func__, self, para);
    return working3_return;
}
esp_gmf_job_err_t cleanup1(void *self, void *para)
{
    vTaskDelay(200 / portTICK_PERIOD_MS);
    printf("%s,%p,%p\r\n", __func__, self, para);
    return cleanup1_return;
}
esp_gmf_job_err_t cleanup2(void *self, void *para)
{
    vTaskDelay(200 / portTICK_PERIOD_MS);
    printf("%s,%p,%p\r\n", __func__, self, para);
    return cleanup2_return;
}
esp_gmf_job_err_t cleanup3(void *self, void *para)
{
    vTaskDelay(100 / portTICK_PERIOD_MS);
    printf("%s,%p,%p\r\n", __func__, self, para);
    return cleanup3_return;
}

static esp_gmf_err_t esp_gmf_task_evt(esp_gmf_event_pkt_t *evt, void *ctx)
{
    esp_gmf_task_handle_t tsk = evt->from;
    ESP_LOGI(TAG, "TASK EVT, tsk:%s-%p, t:%x, sub:%s, pld:%p, sz:%d",
             OBJ_GET_TAG(tsk), evt->from, evt->type, esp_gmf_event_get_state_str(evt->sub), evt->payload, evt->payload_size);
    if (evt->type == ESP_GMF_EVT_TYPE_LOADING_JOB) {
        switch (evt->sub) {
            case ESP_GMF_EVENT_STATE_ERROR:
            case ESP_GMF_EVENT_STATE_STOPPED:
            case ESP_GMF_EVENT_STATE_FINISHED:
                esp_gmf_task_register_ready_job(tsk, NULL, cleanup1, ESP_GMF_JOB_TIMES_ONCE, NULL, true);
                esp_gmf_task_register_ready_job(tsk, NULL, cleanup2, ESP_GMF_JOB_TIMES_ONCE, NULL, true);
                esp_gmf_task_register_ready_job(tsk, NULL, cleanup3, ESP_GMF_JOB_TIMES_ONCE, NULL, true);

                // prepare1_return = 0;
                // prepare2_return = 0;
                // prepare3_return = 0;
                // working1_return = 0;
                // working2_return = 0;
                // working3_return = 0;
                // cleanup1_return = 0;
                // cleanup2_return = 0;
                // cleanup3_return = 0;
                break;
        }
    }
    return ESP_GMF_ERR_OK;
}

TEST_CASE("Working is done", "ESP_GMF_TASK")
{
    esp_log_level_set("*", ESP_LOG_INFO);

    esp_gmf_task_cfg_t cfg = DEFAULT_ESP_GMF_TASK_CONFIG();
    cfg.ctx = NULL;
    cfg.cb = NULL;
    esp_gmf_task_handle_t hd = NULL;

    esp_gmf_task_init(&cfg, &hd);
    esp_gmf_task_register_ready_job(hd, NULL, prepare1, ESP_GMF_JOB_TIMES_ONCE, NULL, false);
    esp_gmf_task_register_ready_job(hd, NULL, prepare2, ESP_GMF_JOB_TIMES_ONCE, NULL, false);
    esp_gmf_task_register_ready_job(hd, NULL, prepare3, ESP_GMF_JOB_TIMES_ONCE, NULL, false);

    esp_gmf_task_register_ready_job(hd, NULL, working1, ESP_GMF_JOB_TIMES_INFINITE, NULL, false);
    esp_gmf_task_register_ready_job(hd, NULL, working2, ESP_GMF_JOB_TIMES_INFINITE, NULL, false);
    esp_gmf_task_register_ready_job(hd, NULL, working3, ESP_GMF_JOB_TIMES_INFINITE, NULL, false);

    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_task_run(hd));
    vTaskDelay(200 / portTICK_PERIOD_MS);

    ESP_LOGW(TAG, "SET working1_return DONE");
    esp_gmf_task_register_ready_job(hd, NULL, cleanup1, ESP_GMF_JOB_TIMES_ONCE, NULL, true);
    working1_return = ESP_GMF_JOB_ERR_DONE;
    vTaskDelay(100 / portTICK_PERIOD_MS);

    ESP_LOGW(TAG, "SET working2_return DONE");
    esp_gmf_task_register_ready_job(hd, NULL, cleanup2, ESP_GMF_JOB_TIMES_ONCE, NULL, true);
    working2_return = ESP_GMF_JOB_ERR_DONE;
    vTaskDelay(100 / portTICK_PERIOD_MS);

    ESP_LOGW(TAG, "SET working3_return DONE");
    esp_gmf_task_register_ready_job(hd, NULL, cleanup3, ESP_GMF_JOB_TIMES_ONCE, NULL, true);
    working3_return = ESP_GMF_JOB_ERR_DONE;
    vTaskDelay(100 / portTICK_PERIOD_MS);

    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_task_deinit(hd));
    ESP_GMF_MEM_SHOW(TAG);
    working3_return = 0;
    working2_return = 0;
    working1_return = 0;
}

TEST_CASE("Stoped by stop API", "ESP_GMF_TASK")
{
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_gmf_task_cfg_t cfg = DEFAULT_ESP_GMF_TASK_CONFIG();
    cfg.ctx = NULL;
    cfg.cb = NULL;
    esp_gmf_task_handle_t hd = NULL;
    ESP_GMF_MEM_SHOW(TAG);
    esp_gmf_task_init(&cfg, &hd);

    esp_gmf_task_set_event_func(hd, esp_gmf_task_evt, NULL);

    esp_gmf_task_register_ready_job(hd, NULL, prepare1, ESP_GMF_JOB_TIMES_ONCE, NULL, false);
    esp_gmf_task_register_ready_job(hd, NULL, prepare2, ESP_GMF_JOB_TIMES_ONCE, NULL, false);
    esp_gmf_task_register_ready_job(hd, NULL, prepare3, ESP_GMF_JOB_TIMES_ONCE, NULL, false);

    esp_gmf_task_register_ready_job(hd, NULL, working1, ESP_GMF_JOB_TIMES_INFINITE, NULL, false);
    esp_gmf_task_register_ready_job(hd, NULL, working2, ESP_GMF_JOB_TIMES_INFINITE, NULL, false);
    esp_gmf_task_register_ready_job(hd, NULL, working3, ESP_GMF_JOB_TIMES_INFINITE, NULL, false);

    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_task_run(hd));
    vTaskDelay(200 / portTICK_PERIOD_MS);

    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_task_pause(hd));
    vTaskDelay(200 / portTICK_PERIOD_MS);

    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_task_resume(hd));
    vTaskDelay(200 / portTICK_PERIOD_MS);

    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_task_stop(hd));

    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_task_deinit(hd));
    ESP_GMF_MEM_SHOW(TAG);
}

TEST_CASE("Return error on the PREPARE stage", "ESP_GMF_TASK")
{
    esp_log_level_set("*", ESP_LOG_INFO);

    esp_gmf_task_cfg_t cfg = DEFAULT_ESP_GMF_TASK_CONFIG();
    cfg.ctx = NULL;
    cfg.cb = NULL;
    esp_gmf_task_handle_t hd = NULL;

    esp_gmf_task_init(&cfg, &hd);
    esp_gmf_task_set_event_func(hd, esp_gmf_task_evt, NULL);

    esp_gmf_task_register_ready_job(hd, NULL, prepare1, ESP_GMF_JOB_TIMES_ONCE, NULL, false);
    esp_gmf_task_register_ready_job(hd, NULL, prepare2, ESP_GMF_JOB_TIMES_ONCE, NULL, false);
    esp_gmf_task_register_ready_job(hd, NULL, prepare3, ESP_GMF_JOB_TIMES_ONCE, NULL, false);

    esp_gmf_task_register_ready_job(hd, NULL, working1, ESP_GMF_JOB_TIMES_INFINITE, NULL, false);
    esp_gmf_task_register_ready_job(hd, NULL, working2, ESP_GMF_JOB_TIMES_INFINITE, NULL, false);
    esp_gmf_task_register_ready_job(hd, NULL, working3, ESP_GMF_JOB_TIMES_INFINITE, NULL, false);

    prepare3_return = ESP_GMF_JOB_ERR_FAIL;
    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_task_run(hd));

    vTaskDelay(200 / portTICK_PERIOD_MS);

    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_task_pause(hd));
    vTaskDelay(100 / portTICK_PERIOD_MS);

    TEST_ASSERT_EQUAL(ESP_GMF_ERR_NOT_SUPPORT, esp_gmf_task_resume(hd));
    vTaskDelay(200 / portTICK_PERIOD_MS);

    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_task_stop(hd));
    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_task_deinit(hd));

    ESP_GMF_MEM_SHOW(TAG);
    prepare3_return = 0;
}

TEST_CASE("Return error on the WORKING stage", "ESP_GMF_TASK")
{
    esp_log_level_set("*", ESP_LOG_INFO);
    // esp_log_level_set("ESP_GMF_TASK", ESP_LOG_DEBUG);

    esp_gmf_task_cfg_t cfg = DEFAULT_ESP_GMF_TASK_CONFIG();
    cfg.ctx = NULL;
    cfg.cb = NULL;
    esp_gmf_task_handle_t hd = NULL;

    esp_gmf_task_init(&cfg, &hd);
    esp_gmf_task_set_event_func(hd, esp_gmf_task_evt, NULL);

    esp_gmf_task_register_ready_job(hd, NULL, prepare1, ESP_GMF_JOB_TIMES_ONCE, NULL, false);
    esp_gmf_task_register_ready_job(hd, NULL, prepare2, ESP_GMF_JOB_TIMES_ONCE, NULL, false);
    esp_gmf_task_register_ready_job(hd, NULL, prepare3, ESP_GMF_JOB_TIMES_ONCE, NULL, false);

    esp_gmf_task_register_ready_job(hd, NULL, working1, ESP_GMF_JOB_TIMES_INFINITE, NULL, false);
    esp_gmf_task_register_ready_job(hd, NULL, working2, ESP_GMF_JOB_TIMES_INFINITE, NULL, false);
    esp_gmf_task_register_ready_job(hd, NULL, working3, ESP_GMF_JOB_TIMES_INFINITE, NULL, false);

    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_task_run(hd));
    vTaskDelay(200 / portTICK_PERIOD_MS);

    working2_return = ESP_GMF_JOB_ERR_FAIL;

    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_task_pause(hd));
    vTaskDelay(100 / portTICK_PERIOD_MS);

    TEST_ASSERT_EQUAL(ESP_GMF_ERR_NOT_SUPPORT, esp_gmf_task_resume(hd));
    vTaskDelay(200 / portTICK_PERIOD_MS);

    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_task_stop(hd));
    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_task_deinit(hd));
    ESP_GMF_MEM_SHOW(TAG);
    working2_return = 0;
}

TEST_CASE("Return error on the CLEANUP stage", "ESP_GMF_TASK")
{
    esp_log_level_set("*", ESP_LOG_INFO);

    esp_gmf_task_cfg_t cfg = DEFAULT_ESP_GMF_TASK_CONFIG();
    cfg.ctx = NULL;
    cfg.cb = NULL;
    esp_gmf_task_handle_t hd = NULL;

    esp_gmf_task_init(&cfg, &hd);
    esp_gmf_task_set_event_func(hd, esp_gmf_task_evt, NULL);

    esp_gmf_task_register_ready_job(hd, NULL, prepare1, ESP_GMF_JOB_TIMES_ONCE, NULL, false);
    esp_gmf_task_register_ready_job(hd, NULL, prepare2, ESP_GMF_JOB_TIMES_ONCE, NULL, false);
    esp_gmf_task_register_ready_job(hd, NULL, prepare3, ESP_GMF_JOB_TIMES_ONCE, NULL, false);

    esp_gmf_task_register_ready_job(hd, NULL, working1, ESP_GMF_JOB_TIMES_INFINITE, NULL, false);
    esp_gmf_task_register_ready_job(hd, NULL, working2, ESP_GMF_JOB_TIMES_INFINITE, NULL, false);
    esp_gmf_task_register_ready_job(hd, NULL, working3, ESP_GMF_JOB_TIMES_INFINITE, NULL, false);

    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_task_run(hd));
    vTaskDelay(200 / portTICK_PERIOD_MS);

    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_task_pause(hd));
    vTaskDelay(100 / portTICK_PERIOD_MS);

    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_task_resume(hd));

    cleanup2_return = ESP_GMF_JOB_ERR_FAIL;
    vTaskDelay(200 / portTICK_PERIOD_MS);

    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_task_stop(hd));
    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_task_deinit(hd));

    ESP_GMF_MEM_SHOW(TAG);
    cleanup2_return = 0;
}

TEST_CASE("Return error after call STOP", "ESP_GMF_TASK")
{
    esp_log_level_set("*", ESP_LOG_INFO);
    // esp_log_level_set("ESP_GMF_TASK", ESP_LOG_DEBUG);

    esp_gmf_task_cfg_t cfg = DEFAULT_ESP_GMF_TASK_CONFIG();
    cfg.ctx = NULL;
    cfg.cb = NULL;
    esp_gmf_task_handle_t hd = NULL;

    esp_gmf_task_init(&cfg, &hd);
    esp_gmf_task_set_event_func(hd, esp_gmf_task_evt, NULL);

    esp_gmf_task_register_ready_job(hd, NULL, prepare1, ESP_GMF_JOB_TIMES_ONCE, NULL, false);
    esp_gmf_task_register_ready_job(hd, NULL, prepare2, ESP_GMF_JOB_TIMES_ONCE, NULL, false);
    esp_gmf_task_register_ready_job(hd, NULL, prepare3, ESP_GMF_JOB_TIMES_ONCE, NULL, false);

    esp_gmf_task_register_ready_job(hd, NULL, working1, ESP_GMF_JOB_TIMES_INFINITE, NULL, false);
    esp_gmf_task_register_ready_job(hd, NULL, working2, ESP_GMF_JOB_TIMES_INFINITE, NULL, false);
    esp_gmf_task_register_ready_job(hd, NULL, working3, ESP_GMF_JOB_TIMES_INFINITE, NULL, false);

    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_task_run(hd));
    vTaskDelay(200 / portTICK_PERIOD_MS);

    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_task_pause(hd));
    vTaskDelay(100 / portTICK_PERIOD_MS);

    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_task_resume(hd));
    vTaskDelay(200 / portTICK_PERIOD_MS);

    cleanup2_return = ESP_GMF_JOB_ERR_FAIL;
    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_task_stop(hd));

    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, esp_gmf_task_deinit(hd));

    ESP_GMF_MEM_SHOW(TAG);
    cleanup2_return = 0;
}
