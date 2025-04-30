/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include <stdint.h>
#include <stdatomic.h>
#include "esp_gmf_event.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief  Atomic declare and operations
 */
#define ATOM_VAR                    atomic_int
#define GET_ATOM(v)                 atomic_load(&v);
#define SET_ATOM(atom, v)           atomic_store(&atom, v)
#define ATOM_SET_BITS(atom, bits)   atomic_fetch_or(&atom, bits)
#define ATOM_CLEAR_BITS(atom, bits) atomic_fetch_and(&atom, ~bits)

#define GMF_VIDEO_BREAK_ON_FAIL(ret) \
    if (ret != ESP_GMF_ERR_OK) {     \
        break;                       \
    }

#define GMF_VIDEO_ALIGN_UP(size, align) (((size) + ((align) - 1)) & ~((align) - 1))

/**
 * @brief  Get video format string representation
 *
 * @param[in]   format  Video format FourCC representation
 *
 * @return
 *       - "none"  Not valid codec type
 *       - Others  String representation of codec
 */
const char *esp_gmf_video_get_format_string(uint32_t format);

/**
 * @brief  Common video event handler
 *
 * @note  This API implement basic handler of GMF report info event, element should derive from `esp_gmf_video_element_t`
 *
 * @param[in]  evt  GMT event packet
 * @param[in]  ctx  Event context
 *
 * @return
 *       - ESP_GMF_ERR_OK  On success
 */
esp_gmf_err_t esp_gmf_video_handle_events(esp_gmf_event_pkt_t *evt, void *ctx);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
