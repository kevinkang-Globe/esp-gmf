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

#include "esp_err.h"
#include "esp_gmf_info.h"
#include "esp_gmf_audio_element.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

static inline void gmf_audio_update_snd_info(void *self, uint32_t sample_rate, uint8_t bits, uint8_t channel)
{
    esp_gmf_info_sound_t snd_info = {0};
    esp_gmf_audio_el_get_snd_info(self, &snd_info);
    snd_info.sample_rates = sample_rate;
    snd_info.channels     = channel;
    snd_info.bits         = bits;
    esp_gmf_audio_el_set_snd_info(self, &snd_info);
    esp_gmf_element_notify_snd_info(self, &snd_info);
}

#define GMF_AUDIO_UPDATE_SND_INFO(self, sample_rate, bits, channel) gmf_audio_update_snd_info(self, sample_rate, bits, channel)

#define GMF_AUDIO_INPUT_SAMPLE_NUM (256)

#ifdef __cplusplus
}
#endif  /* __cplusplus */
