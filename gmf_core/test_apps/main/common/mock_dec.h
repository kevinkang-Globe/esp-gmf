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

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

typedef void *mock_dec_handle_t;

typedef struct {
    uint8_t      a;
    uint32_t     b;
    uint16_t     c;
} mock_args_ldata_t;

#pragma pack(push, 1)
typedef struct {
    uint8_t      d;
    uint32_t     e;
    uint16_t     f;
} mock_args_hdata_t;
#pragma pack(pop)

typedef struct {
    mock_args_ldata_t    first;
    mock_args_hdata_t    second;
    uint16_t             value;
} mock_dec_desc_t;

typedef struct {
    mock_dec_desc_t     desc;
    char                label[16];
} mock_dec_el_args_t;

typedef struct {
    uint32_t  type;
    uint32_t  fc;
    float     q;
    float     gain;
} mock_para_t;

esp_err_t mock_dec_open(mock_dec_handle_t *handle);
esp_err_t mock_dec_process(mock_dec_handle_t handle);
esp_err_t mock_dec_close(mock_dec_handle_t handle);

esp_err_t mock_dec_set_para(mock_dec_handle_t handle, uint8_t index, mock_para_t *para);

esp_err_t mock_dec_get_para(mock_dec_handle_t handle, uint8_t index, mock_para_t *para);

esp_err_t mock_dec_set_info(mock_dec_handle_t handle, uint32_t sample_rate, uint16_t ch, uint16_t bits);

esp_err_t mock_dec_get_info(mock_dec_handle_t handle, uint32_t *sample_rate, uint16_t *ch, uint16_t *bits);

#ifdef __cplusplus
}
#endif  /* __cplusplus */