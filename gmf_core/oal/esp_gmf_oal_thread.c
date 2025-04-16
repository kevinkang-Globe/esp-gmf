/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_gmf_oal_mem.h"
#include "esp_gmf_oal_thread.h"

static const char *TAG = "ESP_GMF_THREAD";

/**
 * @brief  Structure representing a GMF thread
 *         Holds information about a GMF thread, including its handle and whether it is allocated in external RAM
 */
typedef struct {
    TaskHandle_t  handle;          /*!< Handle to the created task */
    uint8_t       is_ext_ram : 1;  /*!< Flag indicating if the thread is allocated in external RAM (1 if true, 0 if false) */
} esp_gmf_thread_t;

esp_gmf_err_t esp_gmf_oal_thread_create(esp_gmf_oal_thread_t *p_handle, const char *name, void (*main_func)(void *arg), void *arg,
                                        uint32_t stack, int prio, bool stack_in_ext, int core_id)
{
    esp_gmf_thread_t *thread = (esp_gmf_thread_t *)esp_gmf_oal_calloc(1, sizeof(esp_gmf_thread_t));
    if (thread == NULL) {
        ESP_LOGE(TAG, "No memory to create GMF thread, %s", name);
        return ESP_GMF_ERR_MEMORY_LACK;
    }

    if (stack_in_ext && esp_gmf_oal_mem_spiram_stack_is_enabled()) {
        BaseType_t ret = xTaskCreatePinnedToCoreWithCaps(main_func, name, stack, arg, prio, &thread->handle,
                                                         core_id, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (ret != pdPASS) {
            ESP_LOGE(TAG, "Error creating task with PSRAM, %s", name);
            return ESP_GMF_ERR_FAIL;
        }
        thread->is_ext_ram = 1;
    } else {
        if (stack_in_ext) {
            ESP_LOGW(TAG, "Make sure selected the `CONFIG_SPIRAM_BOOT_INIT` and `CONFIG_SPIRAM_ALLOW_STACK_EXTERNAL_MEMORY` by `make menuconfig`");
        }
        if (xTaskCreatePinnedToCore(main_func, name, stack, arg, prio, &thread->handle, core_id) != pdPASS) {
            ESP_LOGE(TAG, "Error creating task with RAM, %s", name);
            return ESP_GMF_ERR_FAIL;
        } else {
            ESP_LOGI(TAG, "The %s created on internal memory", name);
        }
        thread->is_ext_ram = 0;
    }
    *p_handle = thread;
    return ESP_GMF_ERR_OK;
}

esp_gmf_err_t esp_gmf_oal_thread_delete(esp_gmf_oal_thread_t p_handle)
{
    ESP_GMF_NULL_CHECK(TAG, p_handle, return ESP_GMF_ERR_INVALID_ARG);
    esp_gmf_thread_t *thread = (esp_gmf_thread_t *)p_handle;
    TaskHandle_t handle = thread->handle;
    if (thread->is_ext_ram) {
        thread->is_ext_ram = false;
        thread->handle = NULL;
        esp_gmf_oal_free(thread);
        vTaskDeleteWithCaps(handle);
    } else {
        thread->handle = NULL;
        esp_gmf_oal_free(thread);
        vTaskDelete(handle);
    }
    return ESP_GMF_ERR_OK;  /* Control never reach here if this is self delete */
}
