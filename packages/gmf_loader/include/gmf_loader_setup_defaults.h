/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#pragma once

#include "esp_gmf_err.h"
#include "esp_gmf_pool.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Register the GMF I/O selected in sdkconfig into the GMF pool
 *
 * @param[in]   pool  Handle to the GMF pool
 *
 * @return
 *       - ESP_GMF_ERR_OK           Success
 *       - ESP_GMF_ERR_MEMORY_LACK  Memory allocation failed
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid argument
 */
esp_gmf_err_t gmf_loader_setup_io_default(esp_gmf_pool_handle_t pool);

/**
 * @brief  Cleans up and releases resources used by GMF I/O
 *
 * @param[in]   pool  Handle to the GMF pool
 *
 * @return
 *       - ESP_GMF_ERR_OK           Success
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid argument
 */
esp_gmf_err_t gmf_loader_teardown_io_default(esp_gmf_pool_handle_t pool);

/**
 * @brief  Register the codec elements selected in sdkconfig into the GMF pool
 *
 * @note This will register `esp_audio_codec`'s default interface at the first time invoked,
 *       and will keep a counter to manage the lifecycle of the registered interface
 *       The registered interface of `esp_audio_codec` will be unregistered
 *       with `gmf_loader_teardown_audio_codec_default()` when the counter reaches 0
 *
 * @param[in]   pool  Handle to the GMF pool
 *
 * @return
 *       - ESP_GMF_ERR_OK           Success
 *       - ESP_GMF_ERR_MEMORY_LACK  Memory allocation failed
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid argument
 */
esp_gmf_err_t gmf_loader_setup_audio_codec_default(esp_gmf_pool_handle_t pool);

/**
 * @brief  Cleans up and releases resources used by codec elements
 *
 * @param[in]   pool  Handle to the GMF pool
 *
 * @return
 *       - ESP_GMF_ERR_OK           Success
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid argument
 */
esp_gmf_err_t gmf_loader_teardown_audio_codec_default(esp_gmf_pool_handle_t pool);

/**
 * @brief  Register the effect elements selected in sdkconfig into the GMF pool
 *
 * @param[in]   pool  Handle to the GMF pool
 *
 * @return
 *       - ESP_GMF_ERR_OK           Success
 *       - ESP_GMF_ERR_MEMORY_LACK  Memory allocation failed
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid argument
 */
esp_gmf_err_t gmf_loader_setup_audio_effects_default(esp_gmf_pool_handle_t pool);

/**
 * @brief  Cleans up and releases resources used by effect elements
 *
 * @param[in]   pool  Handle to the GMF pool
 *
 * @return
 *       - ESP_GMF_ERR_OK           Success
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid argument
 */
esp_gmf_err_t gmf_loader_teardown_audio_effects_default(esp_gmf_pool_handle_t pool);

/**
 * @brief  Register the AI Audio elements selected in sdkconfig into the GMF pool
 *
 * @note   `esp_gmf_afe_manager` will be create automatically if AFE element is enabled in sdkconfig,
 *         so `gmf_loader_teardown_ai_audio` is used to clean up
 *         More than one `esp_gmf_afe_manager` is meaningless, so `gmf_loader_setup_ai_audio`
 *         will print a warning log if `esp_gmf_afe_manager` already exists
 *
 * @param[in]   pool  Handle to the GMF pool
 *
 * @return
 *       - ESP_GMF_ERR_OK           Success
 *       - ESP_GMF_ERR_MEMORY_LACK  Memory allocation failed
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid argument
 */
esp_gmf_err_t gmf_loader_setup_ai_audio_default(esp_gmf_pool_handle_t pool);

/**
 * @brief  Cleans up and releases resources used by AI audio elements
 *
 * @param[in]   pool  Handle to the GMF pool
 *
 * @return
 *       - ESP_GMF_ERR_OK             Success
 *       - ESP_GMF_ERR_INVALID_ARG    Invalid argument
 *       - ESP_GMF_ERR_INVALID_STATE  AI audio context is NULL
 */
esp_gmf_err_t gmf_loader_teardown_ai_audio_default(esp_gmf_pool_handle_t pool);

/**
 * @brief  Register the video codec elements selected in sdkconfig into the GMF pool
 *
 * @note This will register video codec's default interface at the first time invoked,
 *       and will keep a counter to manage the lifecycle of the registered interface.
 *       The registered interface of video codec will be unregistered
 *       with `gmf_loader_teardown_video_default()` when the counter reaches 0
 *
 * @param[in]   pool  Handle to the GMF pool
 *
 * @return
 *       - ESP_GMF_ERR_OK           Success
 *       - ESP_GMF_ERR_MEMORY_LACK  Memory allocation failed
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid argument
 */
esp_gmf_err_t gmf_loader_setup_video_codec_default(esp_gmf_pool_handle_t pool);

/**
 * @brief  Cleans up and releases resources used by video codec elements
 *
 * @param[in]   pool  Handle to the GMF pool
 *
 * @return
 *       - ESP_GMF_ERR_OK           Success
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid argument
 */
esp_gmf_err_t gmf_loader_teardown_video_codec_default(esp_gmf_pool_handle_t pool);

/**
 * @brief  Register the video effect elements selected in sdkconfig into the GMF pool
 *
 * @note  This function will initialize the following effects if enabled:
 *        - PPA: Hardware Pixel Processing Accelerator
 *        - FPS Convert: Frame rate conversion
 *        - Overlay: Video overlay effects
 *        - Color Convert: Convert between different color formats and spaces and it is implemented in software
 *        - Rotate: Rotate video frames by any angle using software and it is implemented in software
 *        - Scale: Resize video frames with different algorithms using software and it is implemented in software
 *        - Crop: Extract regions from video frames using software and it is implemented in software
 *
 * @param[in]  pool  Handle to the GMF pool
 *
 * @return
 *       - ESP_GMF_ERR_OK           Success
 *       - ESP_GMF_ERR_MEMORY_LACK  Memory allocation failed
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid argument
 */
esp_gmf_err_t gmf_loader_setup_video_effects_default(esp_gmf_pool_handle_t pool);

/**
 * @brief  Cleans up and releases resources used by video effect elements
 *
 * @param[in]   pool  Handle to the GMF pool
 *
 * @return
 *       - ESP_GMF_ERR_OK           Success
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid argument
 */
esp_gmf_err_t gmf_loader_teardown_video_effects_default(esp_gmf_pool_handle_t pool);

/**
 * @brief  Initialize and register GMF elements to the GMF pool
 *
 *         This function initializes the GMF elements based on the sdkconfig configuration
 *         and registers them into the specified GMF pool for subsequent use. The elements
 *         include I/O, codec, effects and AI audio elements if enabled in sdkconfig
 *
 * @param[in]  pool  Handle to the GMF pool where elements will be registered
 *
 * @return
 *       - ESP_GMF_ERR_OK           Success
 *       - ESP_GMF_ERR_MEMORY_LACK  Memory allocation failed
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid argument (e.g. NULL pool handle)
 */
esp_gmf_err_t gmf_loader_setup_all_defaults(esp_gmf_pool_handle_t pool);

/**
 * @brief  Cleans up and releases resources used during pool setup
 *
 * @param[in]   pool  Handle to the GMF pool
 *
 * @return
 *       - ESP_GMF_ERR_OK           Success
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid argument
 */
esp_gmf_err_t gmf_loader_teardown_all_defaults(esp_gmf_pool_handle_t pool);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
