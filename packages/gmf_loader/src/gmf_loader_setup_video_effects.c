/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include "esp_gmf_err.h"
#include "esp_gmf_pool.h"
#include "esp_gmf_video_ppa.h"
#include "esp_gmf_video_fps_cvt.h"
#include "esp_gmf_video_overlay.h"

static const char *TAG = "GMF_SETUP_VID_EFFECTS";

#if defined(CONFIG_GMF_VIDEO_EFFECTS_INIT_PPA)
static esp_gmf_err_t gmf_loader_setup_default_video_ppa(esp_gmf_pool_handle_t pool)
{
    ESP_GMF_NULL_CHECK(TAG, pool, return ESP_GMF_ERR_INVALID_ARG);

    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
    esp_gmf_element_handle_t ppa = NULL;
    ret = esp_gmf_video_ppa_init(NULL, &ppa);
    ESP_GMF_RET_ON_ERROR(TAG, ret, return ret, "Failed to init video ppa");
    ret = esp_gmf_pool_register_element(pool, ppa, NULL);
    ESP_GMF_RET_ON_ERROR(TAG, ret, {esp_gmf_element_deinit(ppa); return ret;}, "Failed to register video ppa");
    return ret;
}
#endif  /* CONFIG_GMF_VIDEO_EFFECTS_INIT_PPA */

#if defined(CONFIG_GMF_VIDEO_EFFECTS_INIT_FPS_CONVERT)
static esp_gmf_err_t gmf_loader_setup_default_video_fps_cvt(esp_gmf_pool_handle_t pool)
{
    ESP_GMF_NULL_CHECK(TAG, pool, return ESP_GMF_ERR_INVALID_ARG);

    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
    esp_gmf_element_handle_t cvt = NULL;
    ret = esp_gmf_video_fps_cvt_init(NULL, &cvt);
    ESP_GMF_RET_ON_ERROR(TAG, ret, return ret, "Failed to init video fps cvt");
    ret = esp_gmf_pool_register_element(pool, cvt, NULL);
    ESP_GMF_RET_ON_ERROR(TAG, ret, {esp_gmf_element_deinit(cvt); return ret;}, "Failed to register video fps cvt");
    return ret;
}
#endif  /* CONFIG_GMF_VIDEO_EFFECTS_INIT_FPS_CONVERT */

#if defined(CONFIG_GMF_VIDEO_EFFECTS_INIT_OVERLAY)
static esp_gmf_err_t gmf_loader_setup_default_video_overlay(esp_gmf_pool_handle_t pool)
{
    ESP_GMF_NULL_CHECK(TAG, pool, return ESP_GMF_ERR_INVALID_ARG);

    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
    esp_gmf_element_handle_t overlay = NULL;
    ret = esp_gmf_video_overlay_init(NULL, &overlay);
    ESP_GMF_RET_ON_ERROR(TAG, ret, return ret, "Failed to init video overlay");
    ret = esp_gmf_pool_register_element(pool, overlay, NULL);
    ESP_GMF_RET_ON_ERROR(TAG, ret, {esp_gmf_element_deinit(overlay); return ret;}, "Failed to register video overlay");
    return ret;
}
#endif  /* CONFIG_GMF_VIDEO_EFFECTS_INIT_OVERLAY */

esp_gmf_err_t gmf_loader_setup_video_effects_default(esp_gmf_pool_handle_t pool)
{
    ESP_GMF_NULL_CHECK(TAG, pool, return ESP_GMF_ERR_INVALID_ARG);

    esp_gmf_err_t ret = ESP_GMF_ERR_OK;
#if defined(CONFIG_GMF_VIDEO_EFFECTS_INIT_PPA)
    ret = gmf_loader_setup_default_video_ppa(pool);
    ESP_GMF_RET_ON_ERROR(TAG, ret, return ret, "Failed to setup video ppa");
#endif
#if defined(CONFIG_GMF_VIDEO_EFFECTS_INIT_FPS_CONVERT)
    ret = gmf_loader_setup_default_video_fps_cvt(pool);
    ESP_GMF_RET_ON_ERROR(TAG, ret, return ret, "Failed to setup video fps cvt");
#endif
#if defined(CONFIG_GMF_VIDEO_EFFECTS_INIT_OVERLAY)
    ret = gmf_loader_setup_default_video_overlay(pool);
    ESP_GMF_RET_ON_ERROR(TAG, ret, return ret, "Failed to setup video overlay");
#endif
    return ret;
}

esp_gmf_err_t gmf_loader_teardown_video_effects_default(esp_gmf_pool_handle_t pool)
{
    ESP_GMF_NULL_CHECK(TAG, pool, return ESP_GMF_ERR_INVALID_ARG);

    return ESP_GMF_ERR_OK;
}
