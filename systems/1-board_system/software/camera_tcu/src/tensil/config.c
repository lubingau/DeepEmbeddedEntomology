/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright © 2019-2022 Tensil AI Company */

#include "config.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void tensil_config_parse_object_item_as_size(const cJSON *json,
                                             const char *name, size_t *target) {
    cJSON *item = cJSON_GetObjectItemCaseSensitive(json, name);

    if (cJSON_IsNumber(item) && item->valueint >= 0)
        *target = item->valueint;
}

void tensil_config_parse_object_item_as_uint16(const cJSON *json,
                                               const char *name,
                                               uint16_t *target) {
    cJSON *item = cJSON_GetObjectItemCaseSensitive(json, name);

    if (cJSON_IsNumber(item) && item->valueint <= UINT16_MAX &&
        item->valueint >= 0)
        *target = item->valueint;
}

void tensil_config_parse_object_item_as_string(const cJSON *json,
                                               const char *name, char *target) {
    cJSON *item = cJSON_GetObjectItemCaseSensitive(json, name);

    if (cJSON_IsString(item) &&
        strlen(item->valuestring) <= TENSIL_MAX_STRING_SIZE)
        strcpy(target, item->valuestring);
}

void tensil_config_parse_object_item_as_bool(const cJSON *json,
                                             const char *name, bool *target) {
    cJSON *item = cJSON_GetObjectItemCaseSensitive(json, name);

    if (cJSON_IsBool(item))
        *target = cJSON_IsTrue(item);
}
