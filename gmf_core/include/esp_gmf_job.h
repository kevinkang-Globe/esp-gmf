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

#pragma once
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define ESP_GMF_JOB_LABLE_MAX_LEN (64)  // The maximum length of tag including the '\0' is 64 byte.
#define ESP_GMF_JOB_STR_OPEN      ("_open")
#define ESP_GMF_JOB_STR_PROCESS   ("_proc")
#define ESP_GMF_JOB_STR_CLOSE     ("_close")

/**
 * @brief  This enumeration defines different states that a GMF job can be in
 */
typedef enum {
    ESP_GMF_JOB_STATUS_SUSPENDED = 0,  /*!< The job is suspended */
    ESP_GMF_JOB_STATUS_READY     = 1,  /*!< The job is ready */
    ESP_GMF_JOB_STATUS_RUNNING   = 2   /*!< The job is running */
} esp_gmf_job_status_t;

/**
 * @brief  This enumeration specifies how many times a GMF job should be executed
 */
typedef enum {
    ESP_GMF_JOB_TIMES_NONE     = 0,  /*!< The job should not be executed */
    ESP_GMF_JOB_TIMES_ONCE     = 1,  /*!< The job should be executed once */
    ESP_GMF_JOB_TIMES_INFINITE = 2   /*!< The job should be executed indefinitely */
} esp_gmf_job_times_t;

/**
 * @brief  This enumeration specifies the error status of a GMF job
 */
typedef enum {
    ESP_GMF_JOB_ERR_DONE     = 2,        /*!< The job has been completed */
    ESP_GMF_JOB_ERR_CONTINUE = 1,        /*!< The job should continue */
    ESP_GMF_JOB_ERR_OK       = ESP_OK,   /*!< The job has executed successfully */
    ESP_GMF_JOB_ERR_FAIL     = ESP_FAIL  /*!< The job has failed to execute */
} esp_gmf_job_err_t;

/**
 * @brief  Function pointer type for GMF job functions
 *
 * @param[in]  self  A pointer to the job itself, allowing the function to access its internal data if needed
 * @param[in]  para  A pointer to additional parameters needed by the job function
 *
 * @return
 *       - A value of type esp_gmf_job_err_t indicating the execution status of the job function
 */
typedef esp_gmf_job_err_t (*esp_gmf_job_func)(void *self, void *para);

/**
 * @brief  This structure represents a job in the GMF
 *         A job encapsulates a function to be executed, along with its context and other properties
 */
typedef struct _esp_gmf_job_t {
    struct _esp_gmf_job_t *prev;   /*!< Pointer to the previous job in the linked list */
    struct _esp_gmf_job_t *next;   /*!< Pointer to the next job in the linked list */
    const char            *label;  /*!< Label identifying the job */
    esp_gmf_job_func       func;   /*!< Function pointer to the job's function */
    void                  *ctx;    /*!< Context pointer to be passed to the job's function */
    esp_gmf_job_times_t    times;  /*!< Times the job should be executed */
    esp_gmf_job_err_t      ret;    /*!< Return value of the job function */
} esp_gmf_job_t;

/**
 * @brief  Concatenate two strings into the target string
 *
 * @param[in,out]  target       Pointer to the target buffer where the concatenated string will be stored
 * @param[in]      target_size  Size of the target buffer
 * @param[in]      src1         Pointer to the null-terminated string to be concatenated from the beginning
 * @param[in]      src2         Pointer to the string to be concatenated
 * @param[in]      src2_len     Length of the src2 string to be concatenated
 */
static inline void esp_gmf_job_str_cat(char *target, int target_size,
                                       const char *src1, const char *src2, int src2_len)
{
    memset(target, 0, target_size);
    snprintf(target, target_size - 1 - src2_len, "%s", src1);
    strcat(target, src2);
}

#ifdef __cplusplus
}
#endif  /* __cplusplus */
