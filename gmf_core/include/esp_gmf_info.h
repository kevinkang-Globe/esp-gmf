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

#include "stdint.h"
#include "esp_gmf_err.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Structure representing metadata information for a GMF element
 */
typedef struct {
    void *content;  /*!< Pointer to the content */
    int   length;   /*!< Length of the content */
    void *ctx;      /*!< User context */
} esp_gmf_info_metadata_t;

/**
 * @brief  Structure representing sound-related information for a GMF element
 */
typedef struct {
    int       sample_rates;  /*!< Sample rate */
    int       bitrate;       /*!< Bits per second */
    uint16_t  channels : 8;  /*!< Number of channels */
    uint16_t  bits     : 8;  /*!< Bit depth */
} esp_gmf_info_sound_t;

/**
 * @brief  Structure representing sound-related information for a GMF element
 */
typedef struct {
    uint32_t  codec;    /*!< Video codec type */
    uint16_t  height;   /*!< Height of the picture */
    uint16_t  width;    /*!< Width of the picture */
    uint16_t  fps;      /*!< Frame per sample */
    uint32_t  bitrate;  /*!< Bits per second */
} esp_gmf_info_video_t;

/**
 * @brief  Structure representing picture-related information for a GMF element
 */
typedef struct {
    uint16_t  height;  /*!< Height of the picture */
    uint16_t  width;   /*!< Width of the picture */
} esp_gmf_info_pic_t;

/**
 * @brief  Structure representing file-related information for a GMF element
 */
typedef struct {
    const char *uri;   /*!< Uniform Resource Identifier (URI) of the file */
    uint64_t    size;  /*!< Total size of the file */
    uint64_t    pos;   /*!< Byte position in the file */
} esp_gmf_info_file_t;

/**
 * @brief  Enum representing the type of information for a GMF element
 */
typedef enum {
    ESP_GMF_INFO_SOUND = 1,  /*!< Sound-related information */
    ESP_GMF_INFO_VIDEO = 2,  /*!< Video-related information */
    ESP_GMF_INFO_FILE  = 3,  /*!< File-related information */
    ESP_GMF_INFO_PIC   = 4,  /*!< Picture-related information */
} esp_gmf_info_type_t;

#ifdef __cplusplus
}
#endif  /* __cplusplus */
