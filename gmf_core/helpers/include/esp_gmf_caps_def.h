/*
 * Espressif Modified MIT License
 *
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., LTD
 *
 * Permission is hereby granted for use **exclusively** with Espressif Systems products.
 * This includes the right to use, copy, modify, merge, publish, distribute, and sublicense
 * the Software, subject to the following conditions:
 *
 * 1. This Software **must be used in conjunction with Espressif Systems products**.
 * 2. The above copyright notice and this permission notice shall be included in all copies
 *    or substantial portions of the Software.
 * 3. Redistribution of the Software in source or binary form **for use with non-Espressif products**
 *    is strictly prohibited.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY CLAIM, DAMAGES, OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


// Convert string to character code
static inline uint64_t gmf_str_to_cc(const char *str, int max_len)
{
    uint64_t result = 0;
    if (str && strlen(str) <= max_len) {
        memcpy(&result, str, strlen(str));
    }
    return result;
}

// Convert 64-bit EIGHTCC to string
static inline void gmf_eightcc_to_str(uint64_t eightcc, char out[9])
{
    for (int i = 0; i < 8; i++) {
        out[i] = (char)((eightcc >> (i * 8)) & 0xFF);
    }
    out[8] = '\0';  // Null-terminate the string
}

// Macro to convert a string to an 8-byte identifier (EIGHTCC)
#define STR_2_EIGHTCC(str) gmf_str_to_cc(str, 8)

// Macro to convert a string to an 4-byte identifier (FOURCC)
#define STR_2_FOURTCC(str) gmf_str_to_cc(str, 4)

// Macro to convert an EIGHTCC code to a string
#define EIGHTCC_2_STR(eightcc) ({             \
    static char eightcc_str[9];               \
    gmf_eightcc_to_str(eightcc, eightcc_str); \
    eightcc_str;                              \
})

/***************************************************************************/
/*                      Definition of Audio Capabilities                   */
/***************************************************************************/
#define ESP_GMF_CAPS_AUDIO_DECODER              STR_2_EIGHTCC("AUDDEC")
#define ESP_GMF_CAPS_AUDIO_ENCODER              STR_2_EIGHTCC("AUDENC")
#define ESP_GMF_CAPS_AUDIO_ALC                  STR_2_EIGHTCC("AUDALC")
#define ESP_GMF_CAPS_AUDIO_BIT_CONVERT          STR_2_EIGHTCC("AUDBTCVT")
#define ESP_GMF_CAPS_AUDIO_CHANNEL_CONVERT      STR_2_EIGHTCC("AUDCHCVT")
#define ESP_GMF_CAPS_AUDIO_RATE_CONVERT         STR_2_EIGHTCC("AUDRTCVT")
#define ESP_GMF_CAPS_AUDIO_MIXER                STR_2_EIGHTCC("AUDMIXER")
#define ESP_GMF_CAPS_AUDIO_EQUALIZER            STR_2_EIGHTCC("AUDEQ")
#define ESP_GMF_CAPS_AUDIO_SONIC                STR_2_EIGHTCC("AUDSONIC")
#define ESP_GMF_CAPS_AUDIO_FADE                 STR_2_EIGHTCC("AUDFADE")
#define ESP_GMF_CAPS_AUDIO_DEINTERLEAVE         STR_2_EIGHTCC("AUDDITLV")
#define ESP_GMF_CAPS_AUDIO_INTERLEAVE           STR_2_EIGHTCC("AUDINTLV")


/***************************************************************************/
/*                      Definition of Video Capabilities                   */
/***************************************************************************/
#define ESP_GMF_CAPS_VIDEO_DECODER              STR_2_EIGHTCC("VIDDEC")
#define ESP_GMF_CAPS_VIDEO_ENCODER              STR_2_EIGHTCC("VIDENC")


#ifdef __cplusplus
}
#endif  /* __cplusplus */
