/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include "unity.h"

#include "test_utils.h"
#include "esp_gmf_err.h"
#include "esp_gmf_pool.h"
#include "gmf_loader_setup_defaults.h"

typedef esp_gmf_err_t (*gmf_loader_func_t)(esp_gmf_pool_handle_t);

typedef struct {
    gmf_loader_func_t setup;
    gmf_loader_func_t teardown;
} gmf_loader_func_pair_t;

static const gmf_loader_func_pair_t gmf_loader_funcs[] = {
    {gmf_loader_setup_io_default,          gmf_loader_teardown_io_default},
    {gmf_loader_setup_audio_codec_default, gmf_loader_teardown_audio_codec_default},
    {gmf_loader_setup_audio_effects_default, gmf_loader_teardown_audio_effects_default},
    {gmf_loader_setup_video_codec_default, gmf_loader_teardown_video_codec_default},
    {gmf_loader_setup_video_effects_default, gmf_loader_teardown_video_effects_default},
};

TEST_CASE("GMF Loader one pool Test", "[GMF_LOADER]")
{
    esp_gmf_pool_handle_t pool = NULL;
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;

    ret = esp_gmf_pool_init(&pool);
    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, ret);

    ret = gmf_loader_setup_all_defaults(pool);
    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, ret);

    ret = gmf_loader_teardown_all_defaults(pool);
    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, ret);

    ret = esp_gmf_pool_deinit(pool);
    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, ret);
}

TEST_CASE("GMF Loader multiple pools Test", "[GMF_LOADER]")
{
    esp_gmf_pool_handle_t pools[2] = {NULL, NULL};
    esp_gmf_err_t ret = ESP_GMF_ERR_OK;

    for (int i = 0; i < 2; ++i) {
        ret = esp_gmf_pool_init(&pools[i]);
        TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, ret);
    }

    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < sizeof(gmf_loader_funcs) / sizeof(gmf_loader_funcs[0]); ++j) {
            if (gmf_loader_funcs[j].setup) {
                ret = gmf_loader_funcs[j].setup(pools[i]);
                TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, ret);
            }
        }
    }

    ret = gmf_loader_setup_ai_audio_default(pools[0]);
    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, ret);
    ret = gmf_loader_teardown_ai_audio_default(pools[0]);
    TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, ret);

    for (int i = 0; i < 2; ++i) {
        for (int j = (int)(sizeof(gmf_loader_funcs) / sizeof(gmf_loader_funcs[0])) - 1; j >= 0; --j) {
            if (gmf_loader_funcs[j].teardown) {
                ret = gmf_loader_funcs[j].teardown(pools[i]);
                TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, ret);
            }
        }
    }
    for (int i = 0; i < 2; ++i) {
        ret = esp_gmf_pool_deinit(pools[i]);
        TEST_ASSERT_EQUAL(ESP_GMF_ERR_OK, ret);
    }
}
