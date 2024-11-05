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

#include <esp_types.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/**
 * @brief  Allocate memory of a specified size
 *
 * @param[in]  size  Size of memory to allocate, in bytes
 *
 * @return
 *       - Pointer  to allocated memory on success
 *       - NULL     if an error occurs
 */
void *esp_gmf_oal_malloc(size_t size);

/**
 * @brief  Allocate memory with specified alignment
 *
 * @param[in]  align  Memory alignment in bytes (must be a power of 2 or 0)
 * @param[in]  size   Size of memory to allocate, in bytes
 *
 * @return
 *       - Pointer  to aligned memory on success
 *       - NULL     if an error occurs
 */
void *esp_gmf_oal_malloc_align(uint8_t align, size_t size);

/**
 * @brief  Free allocated memory
 *
 * @param[in]  ptr  Pointer to the memory to free
 *
 */
void esp_gmf_oal_free(void *ptr);

/**
 * @brief  Allocate zero-initialized memory, if SPI RAM is enabled, memory may be allocated there
 *
 * @param[in]  nmemb  Number of elements to allocate
 * @param[in]  size   Size of each element, in bytes
 *
 * @return
 *       - Pointer  to allocated and zero-initialized memory on success
 *       - NULL     if an error occurs
 */
void *esp_gmf_oal_calloc(size_t nmemb, size_t size);

/**
 * @brief  Allocate zero-initialized memory in internal memory
 *
 * @param[in]  nmemb  Number of elements to allocate
 * @param[in]  size   Size of each element, in bytes
 *
 * @return
 *       - Pointer  to allocated and zero-initialized memory on success
 *       - NULL     if an error occurs
 */
void *esp_gmf_oal_calloc_inner(size_t nmemb, size_t size);

/**
 * @brief  Print the current heap memory status
 *
 * @param[in]  tag   Log tag identifier
 * @param[in]  line  Line number where the function is called
 * @param[in]  func  Name of the function making the call
 *
 */
void esp_gmf_oal_mem_print(const char *tag, int line, const char *func);

/**
 * @brief  Reallocate memory to a new size, if SPI RAM is enabled, the memory may be allocated there
 *
 * @param[in]  ptr   Pointer to previously allocated memory
 * @param[in]  size  New size of memory block, in bytes
 *
 * @return
 *       - Pointer  to the reallocated memory block on success
 *       - NULL     if an error occurs
 */
void *esp_gmf_oal_realloc(void *ptr, size_t size);

/**
 * @brief  Duplicate a string in newly allocated memory
 *
 * @param[in]  str  String to duplicate
 *
 * @return
 *       - Pointer  to the duplicated string in allocated memory
 *       - NULL     if an error occurs
 */
char *esp_gmf_oal_strdup(const char *str);

/**
 * @brief  Check if SPI RAM is enabled
 *
 * @return
 *       - true   if SPI RAM is enabled
 *       - false  if SPI RAM is not enabled
 */
bool esp_gmf_oal_mem_spiram_is_enabled(void);

/**
 * @brief  Check if stack allocation on external SPI RAM is enabled
 *
 * @return
 *       - true   if stack allocation on SPI RAM is enabled
 *       - false  if stack allocation on SPI RAM is not enabled
 */
bool esp_gmf_oal_mem_spiram_stack_is_enabled(void);

#define ESP_GMF_MEM_SHOW(x) esp_gmf_oal_mem_print(x, __LINE__, __func__)

#ifdef __cplusplus
}
#endif  /* __cplusplus */
