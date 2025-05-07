/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "esp_gmf_pipeline.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void play_pause_single_file( esp_gmf_pipeline_handle_t pipe, const char *uri);

#ifdef __cplusplus
}
#endif /* __cplusplus */
