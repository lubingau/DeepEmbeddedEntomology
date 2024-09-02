/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright © 2019-2022 Tensil AI Company */

#include "architecture.h"

#include <stdint.h>
#include <string.h>

#include "config.h"
#include "error.h"

#ifdef TENSIL_PLATFORM_ENABLE_STDIO
#include "cJSON.h"
#endif

#ifdef TENSIL_PLATFORM_ENABLE_FILE_SYSTEM
#include "ff.h"
#endif

#define JSMN_TOKEN_POOL_SIZE 64

bool tensil_architecture_is_valid(const struct tensil_architecture *arch) {
    return (arch->array_size > 0 &&
            arch->data_type > TENSIL_DATA_TYPE_INVALID &&
            arch->local_depth > 0 && arch->accumulator_depth > 0 &&
            arch->dram0_depth > 0 && arch->dram1_depth > 0 &&
            arch->stride0_depth > 0 && arch->stride1_depth > 0 &&
            arch->simd_registers_depth > 0);
}

bool tensil_architecture_is_compatible(
    const struct tensil_architecture *driver_arch,
    const struct tensil_architecture *model_arch) {
    // TODO: can be less strict, e.g. if instruction layout is the same and
    // driver->depth >= model->depth
    return (driver_arch->array_size == model_arch->array_size &&
            driver_arch->data_type == model_arch->data_type &&
            driver_arch->local_depth == model_arch->local_depth &&
            driver_arch->accumulator_depth == model_arch->accumulator_depth &&
            driver_arch->dram0_depth == model_arch->dram0_depth &&
            driver_arch->dram1_depth == model_arch->dram1_depth &&
            driver_arch->stride0_depth == model_arch->stride0_depth &&
            driver_arch->stride1_depth == model_arch->stride1_depth &&
            driver_arch->simd_registers_depth ==
                model_arch->simd_registers_depth);
}

#ifdef TENSIL_PLATFORM_ENABLE_STDIO

static void parse_object_item_as_data_type(const cJSON *json, const char *name,
                                           enum tensil_data_type *target) {
    cJSON *item = cJSON_GetObjectItemCaseSensitive(json, name);

    if (cJSON_IsString(item))
        if (strcmp(item->valuestring, "FP16BP8") == 0)
            *target = TENSIL_DATA_TYPE_FP16BP8;
}

void tensil_architecture_parse(struct tensil_architecture *arch,
                               const cJSON *json) {

    memset(arch, 0, sizeof(struct tensil_architecture));

    if (cJSON_IsObject(json)) {
        tensil_config_parse_object_item_as_size(json, "array_size",
                                                &arch->array_size);
        parse_object_item_as_data_type(json, "data_type", &arch->data_type);
        tensil_config_parse_object_item_as_size(json, "local_depth",
                                                &arch->local_depth);
        tensil_config_parse_object_item_as_size(json, "accumulator_depth",
                                                &arch->accumulator_depth);
        tensil_config_parse_object_item_as_size(json, "dram0_depth",
                                                &arch->dram0_depth);
        tensil_config_parse_object_item_as_size(json, "dram1_depth",
                                                &arch->dram1_depth);
        tensil_config_parse_object_item_as_size(json, "stride0_depth",
                                                &arch->stride0_depth);
        tensil_config_parse_object_item_as_size(json, "stride1_depth",
                                                &arch->stride1_depth);
        tensil_config_parse_object_item_as_size(json, "simd_registers_depth",
                                                &arch->simd_registers_depth);
    }
}

#endif
