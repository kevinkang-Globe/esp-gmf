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

#include "mock_dec.h"
#include "esp_gmf_audio_element.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Fake Decoder configurations
 */
typedef struct {
    int               in_buf_size;  /*!< Size of output ringbuffer */
    int               out_buf_size;
    esp_gmf_event_cb  cb;
    const char       *name;
    bool              is_pass;
    bool              is_shared;
} fake_dec_cfg_t;

#define FAKE_DEC_BUFFER_SIZE (5 * 1024)

#define DEFAULT_FAKE_DEC_CONFIG() {        \
    .in_buf_size  = FAKE_DEC_BUFFER_SIZE,  \
    .out_buf_size = FAKE_DEC_BUFFER_SIZE,  \
    .cb           = NULL,                  \
    .name         = NULL,                  \
    .is_pass      = false,                 \
    .is_shared    = true,                  \
}

esp_err_t fake_dec_init(fake_dec_cfg_t *config, esp_gmf_obj_handle_t *handle);
esp_err_t fake_dec_cast(fake_dec_cfg_t *config, esp_gmf_obj_handle_t handle);

#ifdef __cplusplus
}
#endif  /* __cplusplus */