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

#include "esp_gmf_err.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Type of events in GMF
 */
typedef enum {
    ESP_GMF_EVT_TYPE_LOADING_JOB  = 0x1000,  /*!< Loading job event */
    ESP_GMF_EVT_TYPE_CHANGE_STATE = 0x2000,  /*!< State change event */
    ESP_GMF_EVT_TYPE_REPORT_INFO  = 0x3000,  /*!< Information reporting event */
} esp_gmf_event_type_t;

/**
 * @brief  States of GMF events
 */
typedef enum {
    ESP_GMF_EVENT_STATE_NONE        = 0,  /*!< No specific state */
    ESP_GMF_EVENT_STATE_INITIALIZED = 1,  /*!< Initialized state */
    ESP_GMF_EVENT_STATE_OPENING     = 2,  /*!< Opening state */
    ESP_GMF_EVENT_STATE_RUNNING     = 3,  /*!< Running state */
    ESP_GMF_EVENT_STATE_PAUSED      = 4,  /*!< Paused state */
    ESP_GMF_EVENT_STATE_STOPPED     = 5,  /*!< Stopped state */
    ESP_GMF_EVENT_STATE_FINISHED    = 6,  /*!< Finished state */
    ESP_GMF_EVENT_STATE_ERROR       = 7,  /*!< Error state */
} esp_gmf_event_state_t;

/**
 * @brief  Packet containing information about a GMF event
 */
typedef struct {
    void                 *from;          /*!< Pointer to the object sending the event */
    esp_gmf_event_type_t  type;          /*!< Type of the event */
    int                   sub;           /*!< Additional data or subtype */
    void                 *payload;       /*!< Pointer to the payload data */
    int                   payload_size;  /*!< Size of the payload data */
} esp_gmf_event_pkt_t;

/**
 * @brief  Callback function for handling GMF events
 *
 * @param[in]  pkt  Pointer to the packet containing information about the event
 * @param[in]  ctx  Context pointer
 *
 * @return
 *       - esp_gmf_err_t  error code  Indicating success or failure
 */
typedef esp_gmf_err_t (*esp_gmf_event_cb)(esp_gmf_event_pkt_t *pkt, void *ctx);

/**
 * @brief  Structure representing an item in the event queue
 */
typedef struct _esp_gmf_event_item_ {
    struct _esp_gmf_event_item_ *next;  /*!< Pointer to the next event item */
    esp_gmf_event_cb             cb;    /*!< Callback function for the event */
    void                        *ctx;   /*!< Context pointer */
} esp_gmf_event_item_t;

/**
 * @brief  Get the string representation of a GMF event state
 *
 * @param[in]  st  GMF event state
 *
 * @return
 *       - NULL    Out of range
 *       - Others  Const String representation of the state
 */
const char *esp_gmf_event_get_state_str(esp_gmf_event_state_t st);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
