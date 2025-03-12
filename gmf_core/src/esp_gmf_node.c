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

#include "esp_gmf_node.h"
#include "esp_gmf_oal_mem.h"
#include "esp_log.h"
#include <stdio.h>

esp_gmf_node_t *esp_gmf_node_get_head(esp_gmf_node_t *node, int *num)
{
    esp_gmf_node_t *head;
    if (node == NULL) {
        return NULL;
    }
    head = node;
    int cnt = 1;
    while (head->prev != NULL) {
        head = head->prev;
        cnt++;
    }
    *num = cnt;
    return head;
}

esp_gmf_node_t *esp_gmf_node_get_tail(esp_gmf_node_t *root)
{
    esp_gmf_node_t *last;
    if (root == NULL) {
        return NULL;
    }
    last = root;
    while (last->next != NULL) {
        last = last->next;
    }
    return last;
}

void esp_gmf_node_add_last(esp_gmf_node_t *root, esp_gmf_node_t *new_node)
{
    esp_gmf_node_t *last = esp_gmf_node_get_tail(root);
    if (last != NULL) {
        last->next = new_node;
        new_node->prev = last;
        new_node->next = NULL;
    } else {
        ESP_LOGE("NODE", "The root is NULL, %s, %p, %p", __func__, root, new_node);
    }
}

void esp_gmf_node_clear(esp_gmf_node_t **root, node_free del)
{
    while (*root) {
        esp_gmf_node_t *next = (*root)->next;
        if (del) {
            del(*root);
        }
        *root = next;
    }
}

int esp_gmf_node_get_size(esp_gmf_node_t *root)
{
    if (root == NULL) {
        return 0;
    }
    int count = 0;
    while (root->next != NULL) {
        count++;
        root = root->next;
    }
    return count;
}

void esp_gmf_node_insert_after(esp_gmf_node_t *prev, esp_gmf_node_t *new)
{
    if (prev == NULL) {
        return;
    }
    new->next = prev->next;
    prev->next = new;
    new->prev = prev;

    if (new->next) {
        new->next->prev = new;
    }
}

void esp_gmf_node_del_at(esp_gmf_node_t **root, esp_gmf_node_t *del)
{
    if (*root == NULL || del == NULL) {
        return;
    }
    if (*root == del) {
        *root = del->next;
        return;
    }
    if (del->next) {
        del->next->prev = del->prev;
    } else {
        del->prev->next = NULL;
    }

    if (del->prev) {
        del->prev->next = del->next;
    }
    del->next = NULL;
    del->prev = NULL;
}

esp_gmf_node_t *esp_gmf_node_for_next(esp_gmf_node_t *node)
{
    if (node == NULL) {
        return NULL;
    }
    return node->next;
}

esp_gmf_node_t *esp_gmf_node_for_prev(esp_gmf_node_t *node)
{
    if (node == NULL) {
        return NULL;
    }
    return node->prev;
}
