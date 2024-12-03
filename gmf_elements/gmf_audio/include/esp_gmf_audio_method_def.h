/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2025 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

// ALC method
#define ESP_GMF_METHOD_ALC_SET_GAIN          "set_gain"
#define ESP_GMF_METHOD_ALC_SET_GAIN_ARG_IDX  "index"
#define ESP_GMF_METHOD_ALC_SET_GAIN_ARG_GAIN "gain"

#define ESP_GMF_METHOD_ALC_GET_GAIN          "get_gain"
#define ESP_GMF_METHOD_ALC_GET_GAIN_ARG_IDX  "index"
#define ESP_GMF_METHOD_ALC_GET_GAIN_ARG_GAIN "gain"

// BIT CVT method
#define ESP_GMF_METHOD_BIT_CVT_SET_DEST_BITS          "set_dest_bits"
#define ESP_GMF_METHOD_BIT_CVT_SET_DEST_BITS_ARG_BITS "bits"

// CH CVT method
#define ESP_GMF_METHOD_CH_CVT_SET_DEST_CH        "set_dest_ch"
#define ESP_GMF_METHOD_CH_CVT_SET_DEST_CH_ARG_CH "ch"

// RATE CVT method
#define ESP_GMF_METHOD_RATE_CVT_SET_DEST_RATE          "set_dest_rate"
#define ESP_GMF_METHOD_RATE_CVT_SET_DEST_RATE_ARG_RATE "rate"

// EQ method
#define ESP_GMF_METHOD_EQ_SET_PARA               "set_para"
#define ESP_GMF_METHOD_EQ_SET_PARA_ARG_IDX       "index"
#define ESP_GMF_METHOD_EQ_SET_PARA_ARG_PARA      "para"
#define ESP_GMF_METHOD_EQ_SET_PARA_ARG_PARA_FT   "filter_type"
#define ESP_GMF_METHOD_EQ_SET_PARA_ARG_PARA_FC   "fc"
#define ESP_GMF_METHOD_EQ_SET_PARA_ARG_PARA_Q    "q"
#define ESP_GMF_METHOD_EQ_SET_PARA_ARG_PARA_GAIN "gain"

#define ESP_GMF_METHOD_EQ_GET_PARA               "get_para"
#define ESP_GMF_METHOD_EQ_GET_PARA_ARG_IDX       "index"
#define ESP_GMF_METHOD_EQ_GET_PARA_ARG_PARA      "para"
#define ESP_GMF_METHOD_EQ_GET_PARA_ARG_PARA_FT   "filter_type"
#define ESP_GMF_METHOD_EQ_GET_PARA_ARG_PARA_FC   "fc"
#define ESP_GMF_METHOD_EQ_GET_PARA_ARG_PARA_Q    "q"
#define ESP_GMF_METHOD_EQ_GET_PARA_ARG_PARA_GAIN "gain"

#define ESP_GMF_METHOD_EQ_ENABLE_FILTER          "enable_filter"
#define ESP_GMF_METHOD_EQ_ENABLE_FILTER_ARG_IDX  "index"
#define ESP_GMF_METHOD_EQ_ENABLE_FILTER_ARG_PARA "is_enable"

// FADE method
#define ESP_GMF_METHOD_FADE_SET_MODE          "set_mode"
#define ESP_GMF_METHOD_FADE_SET_MODE_ARG_MODE "mode"

#define ESP_GMF_METHOD_FADE_GET_MODE          "get_mode"
#define ESP_GMF_METHOD_FADE_GET_MODE_ARG_MODE "mode"

#define ESP_GMF_METHOD_FADE_RESET "reset_weight"

// MIXER method
#define ESP_GMF_METHOD_MIXER_SET_MODE          "set_mode"
#define ESP_GMF_METHOD_MIXER_SET_MODE_ARG_IDX  "index"
#define ESP_GMF_METHOD_MIXER_SET_MODE_ARG_MODE "mode"

#define ESP_GMF_METHOD_MIXER_SET_INFO          "set_info"
#define ESP_GMF_METHOD_MIXER_SET_INFO_ARG_RATE "rate"
#define ESP_GMF_METHOD_MIXER_SET_INFO_ARG_CH   "ch"
#define ESP_GMF_METHOD_MIXER_SET_INFO_ARG_BITS "bits"

// SONIC method
#define ESP_GMF_METHOD_SONIC_SET_SPEED           "set_speed"
#define ESP_GMF_METHOD_SONIC_SET_SPEED_ARG_SPEED "speed"

#define ESP_GMF_METHOD_SONIC_GET_SPEED           "get_speed"
#define ESP_GMF_METHOD_SONIC_GET_SPEED_ARG_SPEED "speed"

#define ESP_GMF_METHOD_SONIC_SET_PITCH           "set_pitch"
#define ESP_GMF_METHOD_SONIC_SET_PITCH_ARG_PITCH "pitch"

#define ESP_GMF_METHOD_SONIC_GET_PITCH           "get_pitch"
#define ESP_GMF_METHOD_SONIC_GET_PITCH_ARG_PITCH "pitch"

#ifdef __cplusplus
}
#endif  /* __cplusplus */
