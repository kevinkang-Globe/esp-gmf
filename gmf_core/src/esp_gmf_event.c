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

#include "stdlib.h"
#include "esp_gmf_event.h"

static const char *event_state_string[] = {
    "ESP_GMF_EVENT_STATE_NONE",
    "ESP_GMF_EVENT_STATE_INITIALIZED",
    "ESP_GMF_EVENT_STATE_OPENING",
    "ESP_GMF_EVENT_STATE_RUNNING",
    "ESP_GMF_EVENT_STATE_PAUSED",
    "ESP_GMF_EVENT_STATE_STOPPED",
    "ESP_GMF_EVENT_STATE_FINISHED",
    "ESP_GMF_EVENT_STATE_ERROR",
    "",
};

const char *esp_gmf_event_get_state_str(esp_gmf_event_state_t st)
{
    return event_state_string[st];
}
