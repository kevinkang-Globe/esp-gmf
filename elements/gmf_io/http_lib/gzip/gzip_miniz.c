/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "esp_log.h"
#include "gzip_miniz.h"
#include "miniz_inflate.h"

#define TAG              "GZIP_MINIZ"
#define GZIP_HEADER_SIZE (10)

typedef struct {
    gzip_miniz_cfg_t cfg;
    uint8_t         *chunk_ptr;
    int              chunk_filled;
    int              chunk_consumed;
    bool             unzip_error;
    bool             first_data;
    bool             final_data;
    int              head_flag;
    int              extra_len;
    int              head_filled;
    mz_stream        s;
} gzip_miniz_t;

static bool verify_gzip_header(gzip_miniz_t *zip, uint8_t *data, int size)
{
    if (size < GZIP_HEADER_SIZE) {
        return false;
    }
    if (data[0] != 0x1F || data[1] != 0x8B || data[2] != 0x8) {
        return false;
    }
    zip->head_flag = data[3];
    return true;
}

static int gzip_miniz_skip_head(gzip_miniz_t *zip, uint8_t *data, int size)
{
    if (zip->head_flag == 0) {
        return 0;
    }
    int org_size = size;
    if (zip->head_flag & 4) {
        // 2 bytes extra len
        if (zip->head_filled > 2) {
            return -1;
        }
        if (zip->head_filled + size >= 2) {
            int used = 2 - zip->head_filled;
            if (used) {
                size -= used;
                while (used--) {
                    zip->extra_len = zip->extra_len + (*data << (zip->head_filled * 8));
                    zip->head_filled++;
                    data++;
                }
            }
            if (size >= zip->extra_len) {
                size -= zip->extra_len;
                data += zip->extra_len;
                zip->extra_len = 0;
                zip->head_flag &= ~4;
                zip->head_filled = 0;
            } else {
                zip->extra_len -= size;
                return org_size;
            }
        } else {
            for (int i = 0; i < size; i++) {
                zip->extra_len = zip->extra_len + (*data << (zip->head_filled * 8));
                zip->head_filled++;
                data++;
            }
            zip->head_filled += size;
            return org_size;
        }
    }
    if (zip->head_flag & 8) {
        // name
        while (size) {
            size--;
            data++;
            if (*(data - 1) == '\0') {
                zip->head_flag &= ~8;
                break;
            }
        }
        if (size == 0) {
            return org_size;
        }
    }
    if (zip->head_flag & 0x10) {
        // comment
        while (size) {
            size--;
            data++;
            if (*(data - 1) == '\0') {
                zip->head_flag &= ~0x10;
                break;
            }
        }
        if (size == 0) {
            return org_size;
        }
    }
    if (zip->head_flag & 0x2) {
        // CRC16
        if (zip->head_filled > 2) {
            return -1;
        }
        if (zip->head_filled + size >= 2) {
            int used = 2 - zip->head_filled;
            size -= used;
            data += used;
            zip->head_flag &= ~0x2;
        } else {
            zip->head_filled += size;
            return org_size;
        }
    }
    return org_size - size;
}

gzip_miniz_handle_t gzip_miniz_init(gzip_miniz_cfg_t *cfg)
{
    if (cfg->read_cb == NULL) {
        ESP_LOGE(TAG, "Read callback must be provided");
        return NULL;
    }
    gzip_miniz_t *zip = (gzip_miniz_t *) calloc(1, sizeof(gzip_miniz_t));
    if (zip == NULL) {
        ESP_LOGE(TAG, "No memory for instance");
        return NULL;
    }
    zip->cfg = *cfg;
    int chunk_size = cfg->chunk_size ? cfg->chunk_size : 32;
    zip->chunk_ptr = (uint8_t *) malloc(chunk_size);
    if (zip->chunk_ptr == NULL) {
        free(zip);
        ESP_LOGE(TAG, "No memory for chunk");
        return NULL;
    }
    zip->cfg.chunk_size = chunk_size;
    zip->first_data = true;
    mz_inflateInit2(&zip->s, -MZ_DEFAULT_WINDOW_BITS);
    return (gzip_miniz_handle_t)zip;
}

int gzip_miniz_read(gzip_miniz_handle_t h, uint8_t *out, int out_size)
{
    gzip_miniz_t *zip = (gzip_miniz_t *) h;
    if (zip == NULL) {
        return -1;
    }
    if (zip->unzip_error) {
        return -2;
    }
    if (zip->final_data) {
        return 0;
    }
    int size = 0;
    if (zip->first_data == true) {
        zip->first_data = false;
        int size = zip->cfg.read_cb(zip->chunk_ptr, zip->cfg.chunk_size, zip->cfg.ctx);
        if (size < 0) {
            zip->unzip_error = true;
            return -1;
        }
        zip->chunk_filled = size;
        if (verify_gzip_header(zip, zip->chunk_ptr, zip->chunk_filled) == false) {
            zip->unzip_error = true;
            ESP_LOGE(TAG, "Wrong data not match gzip header");
            return -1;
        }
        zip->chunk_consumed = GZIP_HEADER_SIZE;
    }

    while (1) {
        uint8_t *data = zip->chunk_ptr + zip->chunk_consumed;
        size = zip->chunk_filled - zip->chunk_consumed;
        if (size) {
            int skip = gzip_miniz_skip_head(zip, data, size);
            if (skip == 0) {
                break;
            }
            zip->chunk_consumed += skip;
            if (size > skip) {
                break;
            }
        }
        size = zip->cfg.read_cb(zip->chunk_ptr, zip->cfg.chunk_size, zip->cfg.ctx);
        if (size < 0) {
            zip->unzip_error = true;
            ESP_LOGE(TAG, "Fail to read data");
            return -1;
        }
        if (size == 0) {
            zip->final_data = true;
            return 0;
        }
        zip->chunk_filled = size;
        zip->chunk_consumed = 0;
    }

    int org_size = out_size;
    // set input buffer
    mz_stream *s = &zip->s;
    s->next_in = zip->chunk_ptr + zip->chunk_consumed;
    s->avail_in = zip->chunk_filled - zip->chunk_consumed;
    s->next_out = out;
    s->avail_out = out_size;
    while (1) {
        int ret = mz_inflate(s, MZ_SYNC_FLUSH);
        if (ret < 0) {
            zip->unzip_error = true;
            ESP_LOGE(TAG, "Fail to inflate ret %d", ret);
            break;
        }
        int out_consume = out_size - s->avail_out;
        out += out_consume;
        out_size -= out_consume;
        if (ret == MZ_STREAM_END) {
            zip->final_data = true;
            break;
        }
        if (s->avail_out == 0) {
            // All output consumed
            int org_size = zip->chunk_filled - zip->chunk_consumed;
            zip->chunk_consumed += (org_size - s->avail_in);
            break;
        }
        if (s->avail_in == 0) {
            size = zip->cfg.read_cb(zip->chunk_ptr, zip->cfg.chunk_size, zip->cfg.ctx);
            if (size < 0) {
                zip->unzip_error = true;
                ESP_LOGE(TAG, "Fail to read data");
                return -1;
            }
            zip->chunk_filled = size;
            zip->chunk_consumed = 0;
            // Update pointer
            s->next_in = zip->chunk_ptr;
            s->avail_in = zip->chunk_filled - zip->chunk_consumed;
            s->next_out = out;
            s->avail_out = out_size;

        } else {
            // Impossible case
            break;
        }
    }
    return org_size - out_size;
}

int gzip_miniz_deinit(gzip_miniz_handle_t h)
{
    gzip_miniz_t *zip = (gzip_miniz_t *) h;
    if (zip == NULL) {
        return -1;
    }
    mz_inflateEnd(&zip->s);
    free(zip->chunk_ptr);
    free(zip);
    return 0;
}

int gzip_miniz_zip(const uint8_t *input, size_t input_size, uint8_t *out, int out_size)
{
    int pos = 0;
    const uint8_t header[10] = {0x1F, 0x8B, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x03};
    memcpy(out, header, 10);
    pos += 10;

    mz_stream stream = {0};
    if (mz_deflateInit2(&stream, MZ_BEST_COMPRESSION, MZ_DEFLATED, -MZ_DEFAULT_WINDOW_BITS, 1, MZ_DEFAULT_STRATEGY) != MZ_OK) {
        ESP_LOGE(TAG, "Failed to init deflate");
        return -1;
    }

    stream.next_in = (const unsigned char *)input;
    stream.avail_in = input_size;
    int ret;
    do {
        stream.next_out = out + pos;
        stream.avail_out = out_size - pos;
        ret = mz_deflate(&stream, MZ_FINISH);
        if (ret != MZ_OK && ret != MZ_STREAM_END) {
            ESP_LOGE(TAG, "Failed to deflate ret %d", ret);
            mz_deflateEnd(&stream);
            return -2;
        }
        pos += (out_size - pos) - stream.avail_out;
    } while (ret != MZ_STREAM_END);
    // Clean up compression stream
    mz_deflateEnd(&stream);

    uint32_t crc_value = mz_crc32(0, input, input_size);
    memcpy(out + pos, &crc_value, 4);
    pos += 4;
    memcpy(out + pos, &input_size, 4);
    pos += 4;
    return pos;
}
