/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include <sys/time.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_gmf_oal_mem.h"
#include "esp_gmf_oal_sys.h"
#include "esp_memory_utils.h"

static const char *TAG = "ESP_GMF_OAL_SYS";

#define ARRAY_SIZE_OFFSET               8     // Increase this if esp_gmf_oal_sys_get_real_time_stats returns ESP_GMF_ERR_NOT_ENOUGH
#define AUDIO_SYS_TASKS_ELAPSED_TIME_MS 1000  // Period of stats measurement

#ifndef configRUN_TIME_COUNTER_TYPE
#define configRUN_TIME_COUNTER_TYPE uint32_t
#endif

const char *task_state[] = {
    "Running",
    "Ready",
    "Blocked",
    "Suspended",
    "Deleted",
    "invalid state"};

/** @brief
 * "Extr": Allocated task stack from psram, "Intr": Allocated task stack from internal
 */
const char *task_stack[] = {"Extr", "Intr"};

int esp_gmf_oal_sys_get_tick_by_time_ms(int ms)
{
    return (ms / portTICK_PERIOD_MS);
}

int64_t esp_gmf_oal_sys_get_time_ms(void)
{
    struct timeval tmp;
    gettimeofday(&tmp, NULL);
    int64_t milliseconds = tmp.tv_sec * 1000LL + tmp.tv_usec / 1000;
    return milliseconds;
}

esp_gmf_err_t esp_gmf_oal_sys_get_real_time_stats(int elapsed_time_ms)
{
#if (CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID && CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS)
    TaskStatus_t *start_array = NULL, *end_array = NULL;
    UBaseType_t start_array_size, end_array_size;
    configRUN_TIME_COUNTER_TYPE start_run_time, end_run_time;
    configRUN_TIME_COUNTER_TYPE task_elapsed_time;
    float percentage_time;
    esp_gmf_err_t ret;

    //// Allocate array to store current task states
    start_array_size = uxTaskGetNumberOfTasks() + ARRAY_SIZE_OFFSET;
    start_array = esp_gmf_oal_malloc(sizeof(TaskStatus_t) * start_array_size);
    ESP_GMF_MEM_CHECK(TAG, start_array, {
        ret = ESP_GMF_ERR_MEMORY_LACK;
        goto exit;
    });
    // Get current task states
    start_array_size = uxTaskGetSystemState(start_array, start_array_size, &start_run_time);
    if (start_array_size == 0) {
        ESP_LOGE(TAG, "Insufficient array size for uxTaskGetSystemState. Trying increasing ARRAY_SIZE_OFFSET");
        ret = ESP_GMF_ERR_NOT_ENOUGH;
        goto exit;
    }

    vTaskDelay(pdMS_TO_TICKS(elapsed_time_ms));

    // Allocate array to store tasks states post delay
    end_array_size = uxTaskGetNumberOfTasks() + ARRAY_SIZE_OFFSET;
    end_array = esp_gmf_oal_malloc(sizeof(TaskStatus_t) * end_array_size);
    ESP_GMF_MEM_CHECK(TAG, end_array, {
        ret = ESP_GMF_ERR_MEMORY_LACK;
        goto exit;
    });

    // Get post delay task states
    end_array_size = uxTaskGetSystemState(end_array, end_array_size, &end_run_time);
    if (end_array_size == 0) {
        ESP_LOGE(TAG, "Insufficient array size for uxTaskGetSystemState. Trying increasing ARRAY_SIZE_OFFSET");
        ret = ESP_GMF_ERR_FAIL;
        goto exit;
    }

    // Calculate total_elapsed_time in units of run time stats clock period.
    uint32_t total_elapsed_time = (end_run_time - start_run_time);
    if (total_elapsed_time == 0) {
        ESP_LOGE(TAG, "Delay duration too short. Trying increasing AUDIO_SYS_TASKS_ELAPSED_TIME_MS");
        ret = ESP_GMF_ERR_FAIL;
        goto exit;
    }

    ESP_LOGI(TAG, "| Task              | Run Time    | Per | Prio | HWM       | State   | CoreId   | Stack ");

    // Match each task in start_array to those in the end_array
    for (int i = 0; i < start_array_size; i++) {
        for (int j = 0; j < end_array_size; j++) {
            if (start_array[i].xHandle == end_array[j].xHandle) {

                task_elapsed_time = end_array[j].ulRunTimeCounter - start_array[i].ulRunTimeCounter;
                percentage_time = (task_elapsed_time * 100UL) / ((float)total_elapsed_time * portNUM_PROCESSORS);
                bool is_task_inter = esp_ptr_internal((const void *)(pxTaskGetStackStart(start_array[i].xHandle)));
                ESP_LOGI(TAG, "| %-17s | %-11d |%.2f%%  | %-4u | %-9u | %-7s | %-8x | %s",
                         start_array[i].pcTaskName, (int)task_elapsed_time, percentage_time, start_array[i].uxCurrentPriority,
                         (int)start_array[i].usStackHighWaterMark, task_state[(start_array[i].eCurrentState)],
                         start_array[i].xCoreID, task_stack[(int)(is_task_inter)]);

                // Mark that task have been matched by overwriting their handles
                start_array[i].xHandle = NULL;
                end_array[j].xHandle = NULL;
                break;
            }
        }
    }

    // Print unmatched tasks
    for (int i = 0; i < start_array_size; i++) {
        if (start_array[i].xHandle != NULL) {
            ESP_LOGI(TAG, "| %s | Deleted", start_array[i].pcTaskName);
        }
    }
    for (int i = 0; i < end_array_size; i++) {
        if (end_array[i].xHandle != NULL) {
            ESP_LOGI(TAG, "| %s | Created", end_array[i].pcTaskName);
        }
    }
    printf("\n");
    ret = ESP_GMF_ERR_OK;

exit:  // Common return path
    if (start_array) {
        esp_gmf_oal_free(start_array);
        start_array = NULL;
    }
    if (end_array) {
        esp_gmf_oal_free(end_array);
        end_array = NULL;
    }
    return ret;
#else
    ESP_LOGW(TAG, "Please enable `CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID` and `CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS` in menuconfig");
    return ESP_GMF_ERR_FAIL;
#endif  /* (CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID && CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS) */
}
