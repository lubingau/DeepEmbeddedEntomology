/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright © 2019-2022 Tensil AI Company */

#include "driver.h"

#include <malloc.h>
#include <math.h>
#include <string.h>

#ifdef TENSIL_PLATFORM_ENABLE_STDIO
#include <stdio.h>
#endif

#include "dram.h"
#include "instruction_buffer.h"
#include "sample_buffer.h"

#ifdef TENSIL_PLATFORM_ENABLE_STDIO

static const char *ok = "\033[38;2;0;255;00mOK\033[39m";
static const char *failed = "\033[38;2;255;0;00mFAILED\033[39m";

static void fill_dram_with_random_vectors(struct tensil_driver *driver,
                                          enum tensil_dram_bank dram_bank,
                                          size_t offset, size_t stride,
                                          size_t size) {
    uint8_t *bank_ptr = tensil_driver_get_dram_bank_base_ptr(driver, dram_bank);

    if (stride == 0)
        tensil_dram_fill_random(bank_ptr, driver->arch.data_type,
                                offset * driver->arch.array_size,
                                size * driver->arch.array_size);
    else
        for (size_t i = 0; i < size; i++)
            tensil_dram_fill_random(bank_ptr, driver->arch.data_type,
                                    (offset + i * (1 << stride)) *
                                        driver->arch.array_size,
                                    driver->arch.array_size);
}

#define TEST_MAX_BAD_INDEXES_SIZE 32

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

static tensil_error_t
do_memory_test(struct tensil_driver *driver, enum tensil_dram_bank from_bank,
               size_t from_offset, float *from_buffer,
               enum tensil_dram_bank to_bank, size_t to_offset,
               float *to_buffer, size_t size, size_t stride0, size_t stride1,
               size_t *failure_count, size_t *test_count, bool verbose) {
    if (from_offset + size * (1 << MAX(stride0, stride1)) >
            driver->arch.local_depth ||
        to_offset + size * (1 << MAX(stride0, stride1)) >
            driver->arch.local_depth ||
        to_offset + size * (1 << MAX(stride0, stride1)) >
            driver->arch.accumulator_depth)
        return TENSIL_ERROR_NONE;

    uint8_t from_flags;
    switch (from_bank) {
    case TENSIL_DRAM0:
    default:
        from_flags = TENSIL_DATA_MOVE_FLAG_DRAM0_TO_LOCAL;
        break;

    case TENSIL_DRAM1:
        from_flags = TENSIL_DATA_MOVE_FLAG_DRAM1_TO_LOCAL;
        break;
    }

    uint8_t to_flags;
    switch (to_bank) {
    case TENSIL_DRAM0:
    default:
        to_flags = TENSIL_DATA_MOVE_FLAG_LOCAL_TO_DRAM0;
        break;

    case TENSIL_DRAM1:
        to_flags = TENSIL_DATA_MOVE_FLAG_LOCAL_TO_DRAM1;
        break;
    }

    fill_dram_with_random_vectors(driver, from_bank, from_offset, stride1,
                                  size);
    tensil_driver_read_dram_vectors(driver, from_bank, from_offset, stride1,
                                    size, from_buffer);

    tensil_error_t error = tensil_driver_setup_buffer_preamble(driver);

    if (error)
        return error;

    error = tensil_buffer_append_instruction(
        &driver->buffer, &driver->layout, TENSIL_OPCODE_DATA_MOVE, from_flags,
        tensil_instruction_make_operand0(&driver->layout, from_offset, stride0),
        tensil_instruction_make_operand1(&driver->layout, from_offset, stride1),
        size - 1);

    if (error)
        return error;

    error = tensil_buffer_append_instruction(
        &driver->buffer, &driver->layout, TENSIL_OPCODE_DATA_MOVE,
        TENSIL_DATA_MOVE_FLAG_LOCAL_TO_ACC,
        tensil_instruction_make_operand0(&driver->layout, from_offset, stride0),
        tensil_instruction_make_operand1(&driver->layout, from_offset, stride1),
        size - 1);

    if (error)
        return error;

    error = tensil_buffer_append_instruction(
        &driver->buffer, &driver->layout, TENSIL_OPCODE_DATA_MOVE,
        TENSIL_DATA_MOVE_FLAG_ACC_TO_LOCAL,
        tensil_instruction_make_operand0(&driver->layout, to_offset, stride0),
        tensil_instruction_make_operand1(&driver->layout, from_offset, stride1),
        size - 1);

    if (error)
        return error;

    error = tensil_buffer_append_instruction(
        &driver->buffer, &driver->layout, TENSIL_OPCODE_DATA_MOVE, to_flags,
        tensil_instruction_make_operand0(&driver->layout, to_offset, stride0),
        tensil_instruction_make_operand1(&driver->layout, to_offset, stride1),
        size - 1);

    if (error)
        return error;

    error = tensil_driver_setup_buffer_postamble(driver);

    if (error)
        return error;

    error = tensil_driver_run(driver, NULL);

    if (error)
        return error;

    tensil_driver_read_dram_vectors(driver, to_bank, to_offset, stride1, size,
                                    to_buffer);

    size_t bad_indexes[TEST_MAX_BAD_INDEXES_SIZE];
    size_t bad_indexes_size = 0;

    for (size_t k = 0; k < size * driver->arch.array_size; k++)
        if (from_buffer[k] != to_buffer[k]) {
            bad_indexes[bad_indexes_size++] = k;

            if (bad_indexes_size == TEST_MAX_BAD_INDEXES_SIZE)
                break;
        }

    if (bad_indexes_size) {
        (*failure_count)++;

        if (verbose) {
            printf("%s moving %zu vectors from %zu to %zu:\n", failed, size,
                   from_offset, to_offset);

            for (size_t k = 0; k < bad_indexes_size; k++) {
                size_t bad_index = bad_indexes[k];

                printf("\t[%zu]%f!=[%zu]%f\n",
                       (from_offset)*driver->arch.array_size + bad_index,
                       from_buffer[bad_index],
                       (to_offset)*driver->arch.array_size + bad_index,
                       to_buffer[bad_index]);
            }
        }
    }

    (*test_count)++;

    return TENSIL_ERROR_NONE;
}

#define MEMORY_TEST_MIN_SIZE 8
#define MEMORY_TEST_MAX_SIZE (driver->arch.accumulator_depth)
#define MEMORY_TEST_UNTIL_OFFSET (driver->arch.accumulator_depth)
#define MEMORY_TEST_UNTIL_SHIFT 4
#define MEMORY_TEST_UNTIL_STRIDE0 (driver->arch.stride0_depth)
#define MEMORY_TEST_UNTIL_STRIDE1 (driver->arch.stride1_depth)

tensil_error_t tensil_driver_run_memory_test(struct tensil_driver *driver,
                                             enum tensil_dram_bank from_bank,
                                             enum tensil_dram_bank to_bank,
                                             bool verbose) {
    tensil_error_t error = TENSIL_ERROR_NONE;
    float *from_buffer = (float *)malloc(
        MEMORY_TEST_MAX_SIZE * driver->arch.array_size * sizeof(float));
    float *to_buffer = (float *)malloc(MEMORY_TEST_MAX_SIZE *
                                       driver->arch.array_size * sizeof(float));

    if (!from_buffer || !to_buffer) {
        error = TENSIL_DRIVER_ERROR(TENSIL_ERROR_DRIVER_OUT_OF_HEAP_MEMORY,
                                    "Out of heap memory");
        goto cleanup;
    }

    for (size_t size_center = MEMORY_TEST_MIN_SIZE;
         size_center <= MEMORY_TEST_MAX_SIZE; size_center *= 2)
        for (size_t size =
                 size_center == MEMORY_TEST_MIN_SIZE ? 1 : size_center - 1;
             size <= MIN(MEMORY_TEST_MAX_SIZE, size_center + 1); size++) {
            size_t failure_count = 0;
            size_t test_count = 0;

            if (size > 0) {
                printf("%06zu vectors -----------------------\n\tStrides test ",
                       size);
                fflush(stdout);

                for (size_t stride0 = 0; stride0 < MEMORY_TEST_UNTIL_STRIDE0;
                     stride0++)
                    for (size_t stride1 = 0;
                         stride1 < MEMORY_TEST_UNTIL_STRIDE1; stride1++)
                        for (size_t from_offset = 0;
                             from_offset < MEMORY_TEST_UNTIL_SHIFT;
                             from_offset++)
                            for (size_t to_offset = 0;
                                 to_offset < MEMORY_TEST_UNTIL_SHIFT;
                                 to_offset++) {
                                error = do_memory_test(
                                    driver, from_bank, from_offset, from_buffer,
                                    to_bank, to_offset, to_buffer, size,
                                    stride0, stride1, &failure_count,
                                    &test_count, verbose);

                                if (error)
                                    goto cleanup;
                            }

                printf("%s: %zu tests, %zu failures\n",
                       failure_count ? failed : ok, test_count, failure_count);
            }

            printf("\tOffsets test ");
            fflush(stdout);

            for (size_t offset = 0; offset < MEMORY_TEST_UNTIL_OFFSET; offset++)
                for (size_t from_shift = 0;
                     from_shift < MEMORY_TEST_UNTIL_SHIFT; from_shift++)
                    for (size_t to_shift = 0;
                         to_shift < MEMORY_TEST_UNTIL_SHIFT; to_shift++) {
                        size_t from_offset = offset + from_shift;
                        size_t to_offset = offset + to_shift;

                        error = do_memory_test(
                            driver, from_bank, from_offset, from_buffer,
                            to_bank, to_offset, to_buffer, size, 0, 0,
                            &failure_count, &test_count, verbose);

                        if (error)
                            goto cleanup;
                    }

            printf("%s: %zu tests, %zu failures\n", failure_count ? failed : ok,
                   test_count, failure_count);
        }

cleanup:
    free(from_buffer);
    free(to_buffer);

    return error;
}

static float saturate(enum tensil_data_type type, float x) {
    float max = tensil_dram_max_scalar(type);
    float min = tensil_dram_min_scalar(type);

    return x > max ? max : x < min ? min : x;
}

static bool compare_scalars(enum tensil_data_type type, float expected,
                            float actual) {
    float max_error = tensil_dram_max_error_scalar(type);

    return fabs(expected - actual) > max_error;
}

#define ARRAY_TEST_SIZE (driver->arch.accumulator_depth)
#define ARRAY_TEST_IDENTITY_WEIGHT 3.456
#define ARRAY_TEST_BIAS 78.912

#define ARRAY_TEST_INPUT_DRAM0_ADDRESS 0
#define ARRAY_TEST_INPUT_LOCAL_ADDRESS 0

#define ARRAY_TEST_OUTPUT_ACC_ADDRESS 0
#define ARRAY_TEST_OUTPUT_LOCAL_ADDRESS ARRAY_TEST_SIZE
#define ARRAY_TEST_OUTPUT_DRAM0_ADDRESS ARRAY_TEST_SIZE

#define ARRAY_TEST_WEIGHTS_DRAM1_ADDRESS 0
#define ARRAY_TEST_WEIGHTS_LOCAL_ADDRESS (ARRAY_TEST_SIZE * 2)

tensil_error_t tensil_driver_run_array_test(struct tensil_driver *driver,
                                            bool verbose) {
    tensil_error_t error = TENSIL_ERROR_NONE;
    size_t bad_indexes[TEST_MAX_BAD_INDEXES_SIZE];
    size_t bad_indexes_size = 0;

    float *from_buffer = (float *)malloc(
        ARRAY_TEST_SIZE * driver->arch.array_size * sizeof(float));
    float *to_buffer = (float *)malloc(ARRAY_TEST_SIZE *
                                       driver->arch.array_size * sizeof(float));
    float *weights_buffer =
        (float *)malloc(driver->arch.array_size * sizeof(float));

    if (!from_buffer || !to_buffer || !weights_buffer) {
        error = TENSIL_DRIVER_ERROR(TENSIL_ERROR_DRIVER_OUT_OF_HEAP_MEMORY,
                                    "Out of heap memory");
        goto cleanup;
    }

    fill_dram_with_random_vectors(driver, TENSIL_DRAM0,
                                  ARRAY_TEST_INPUT_DRAM0_ADDRESS, 0,
                                  ARRAY_TEST_SIZE);
    tensil_driver_read_dram_vectors(driver, TENSIL_DRAM0,
                                    ARRAY_TEST_INPUT_DRAM0_ADDRESS, 0,
                                    ARRAY_TEST_SIZE, from_buffer);

    for (size_t j = 0; j < driver->arch.array_size; j++)
        weights_buffer[j] = ARRAY_TEST_BIAS;

    // TODO: Use non-identity weights to test all MAC units

    tensil_driver_write_dram_vectors(driver, TENSIL_DRAM1,
                                     ARRAY_TEST_WEIGHTS_DRAM1_ADDRESS, 0, 1,
                                     weights_buffer);

    for (size_t i = 0; i < driver->arch.array_size; i++) {
        for (size_t j = 0; j < driver->arch.array_size; j++)
            if (i == j)
                weights_buffer[j] = ARRAY_TEST_IDENTITY_WEIGHT;
            else
                weights_buffer[j] = 0.0;

        tensil_driver_write_dram_vectors(
            driver, TENSIL_DRAM1, ARRAY_TEST_WEIGHTS_DRAM1_ADDRESS + 1 + i, 0,
            1, weights_buffer);
    }

    error = tensil_driver_setup_buffer_preamble(driver);

    if (error)
        goto cleanup;

    error = tensil_buffer_append_instruction(
        &driver->buffer, &driver->layout, TENSIL_OPCODE_DATA_MOVE,
        TENSIL_DATA_MOVE_FLAG_DRAM1_TO_LOCAL, ARRAY_TEST_WEIGHTS_LOCAL_ADDRESS,
        ARRAY_TEST_WEIGHTS_DRAM1_ADDRESS, driver->arch.array_size);

    if (error)
        goto cleanup;

    error = tensil_buffer_append_instruction(
        &driver->buffer, &driver->layout, TENSIL_OPCODE_LOAD_WEIGHT, 0,
        ARRAY_TEST_WEIGHTS_LOCAL_ADDRESS, driver->arch.array_size, 0);

    if (error)
        goto cleanup;

    error = tensil_buffer_append_instruction(
        &driver->buffer, &driver->layout, TENSIL_OPCODE_DATA_MOVE,
        TENSIL_DATA_MOVE_FLAG_DRAM0_TO_LOCAL, ARRAY_TEST_INPUT_LOCAL_ADDRESS,
        ARRAY_TEST_INPUT_DRAM0_ADDRESS, ARRAY_TEST_SIZE - 1);

    if (error)
        goto cleanup;

    error = tensil_buffer_append_instruction(
        &driver->buffer, &driver->layout, TENSIL_OPCODE_MAT_MUL, 0,
        ARRAY_TEST_INPUT_LOCAL_ADDRESS, ARRAY_TEST_OUTPUT_ACC_ADDRESS,
        ARRAY_TEST_SIZE - 1);

    if (error)
        goto cleanup;

    error = tensil_buffer_append_instruction(
        &driver->buffer, &driver->layout, TENSIL_OPCODE_DATA_MOVE,
        TENSIL_DATA_MOVE_FLAG_ACC_TO_LOCAL, ARRAY_TEST_OUTPUT_LOCAL_ADDRESS,
        ARRAY_TEST_OUTPUT_ACC_ADDRESS, ARRAY_TEST_SIZE - 1);

    if (error)
        goto cleanup;

    error = tensil_buffer_append_instruction(
        &driver->buffer, &driver->layout, TENSIL_OPCODE_DATA_MOVE,
        TENSIL_DATA_MOVE_FLAG_LOCAL_TO_DRAM0, ARRAY_TEST_OUTPUT_LOCAL_ADDRESS,
        ARRAY_TEST_OUTPUT_DRAM0_ADDRESS, ARRAY_TEST_SIZE - 1);

    if (error)
        goto cleanup;

    error = tensil_driver_setup_buffer_postamble(driver);

    if (error)
        goto cleanup;

    error = tensil_driver_run(driver, NULL);

    if (error)
        goto cleanup;

    tensil_driver_read_dram_vectors(driver, TENSIL_DRAM0,
                                    ARRAY_TEST_OUTPUT_DRAM0_ADDRESS, 0,
                                    ARRAY_TEST_SIZE, to_buffer);

    for (size_t k = 0; k < ARRAY_TEST_SIZE * driver->arch.array_size; k++) {
        from_buffer[k] = saturate(
            driver->arch.data_type,
            (from_buffer[k] * ARRAY_TEST_IDENTITY_WEIGHT) + ARRAY_TEST_BIAS);

        if (compare_scalars(driver->arch.data_type, from_buffer[k],
                            to_buffer[k])) {
            bad_indexes[bad_indexes_size++] = k;

            if (bad_indexes_size == TEST_MAX_BAD_INDEXES_SIZE)
                break;
        }
    }

    printf("%s\n", bad_indexes_size ? failed : ok);

    if (bad_indexes_size && verbose)
        for (size_t k = 0; k < bad_indexes_size; k++) {
            size_t bad_index = bad_indexes[k];

            printf("\t at %zu expected=%f, actual=%f\n", bad_index,
                   from_buffer[bad_index], to_buffer[bad_index]);
        }

cleanup:
    free(from_buffer);
    free(to_buffer);
    free(weights_buffer);

    return error;
}

#define SIMD_TEST_SIZE (driver->arch.accumulator_depth / 4)

#define SIMD_TEST_MUL 3.456
#define SIMD_TEST_ADD 78.912

#define SIMD_TEST_INPUT_DRAM0_ADDRESS 0
#define SIMD_TEST_INPUT_ACC_ADDRESS 0
#define SIMD_TEST_INPUT_LOCAL_ADDRESS 0

#define SIMD_TEST_MULS_DRAM1_ADDRESS 0
#define SIMD_TEST_MULS_ACC_ADDRESS SIMD_TEST_SIZE
#define SIMD_TEST_MULS_LOCAL_ADDRESS SIMD_TEST_SIZE

#define SIMD_TEST_ADDS_DRAM1_ADDRESS SIMD_TEST_SIZE
#define SIMD_TEST_ADDS_ACC_ADDRESS (SIMD_TEST_SIZE * 2)
#define SIMD_TEST_ADDS_LOCAL_ADDRESS (SIMD_TEST_SIZE * 2)

#define SIMD_TEST_OUTPUT_ACC_ADDRESS (SIMD_TEST_SIZE * 3)
#define SIMD_TEST_OUTPUT_LOCAL_ADDRESS (SIMD_TEST_SIZE * 3)
#define SIMD_TEST_OUTPUT_DRAM0_ADDRESS SIMD_TEST_SIZE

tensil_error_t tensil_driver_run_simd_test(struct tensil_driver *driver,
                                           bool verbose) {
    tensil_error_t error = TENSIL_ERROR_NONE;
    size_t bad_indexes[TEST_MAX_BAD_INDEXES_SIZE];
    size_t bad_indexes_size = 0;

    float *from_buffer = (float *)malloc(
        SIMD_TEST_SIZE * driver->arch.array_size * sizeof(float));
    float *to_buffer = (float *)malloc(SIMD_TEST_SIZE *
                                       driver->arch.array_size * sizeof(float));
    float *consts_buffer =
        (float *)malloc(driver->arch.array_size * sizeof(float));

    if (!from_buffer || !to_buffer || !consts_buffer) {
        error = TENSIL_DRIVER_ERROR(TENSIL_ERROR_DRIVER_OUT_OF_HEAP_MEMORY,
                                    "Out of heap memory");
        goto cleanup;
    }

    fill_dram_with_random_vectors(
        driver, TENSIL_DRAM0, SIMD_TEST_INPUT_DRAM0_ADDRESS, 0, SIMD_TEST_SIZE);
    tensil_driver_read_dram_vectors(driver, TENSIL_DRAM0,
                                    SIMD_TEST_INPUT_DRAM0_ADDRESS, 0,
                                    SIMD_TEST_SIZE, from_buffer);

    for (size_t j = 0; j < driver->arch.array_size; j++)
        consts_buffer[j] = SIMD_TEST_MUL;

    for (size_t i = 0; i < SIMD_TEST_SIZE; i++) {
        tensil_driver_write_dram_vectors(driver, TENSIL_DRAM1,
                                         SIMD_TEST_MULS_DRAM1_ADDRESS + i, 0, 1,
                                         consts_buffer);
    }

    for (size_t j = 0; j < driver->arch.array_size; j++)
        consts_buffer[j] = SIMD_TEST_ADD;

    for (size_t i = 0; i < SIMD_TEST_SIZE; i++) {
        tensil_driver_write_dram_vectors(driver, TENSIL_DRAM1,
                                         SIMD_TEST_ADDS_DRAM1_ADDRESS + i, 0, 1,
                                         consts_buffer);
    }

    error = tensil_driver_setup_buffer_preamble(driver);

    if (error)
        goto cleanup;

    error = tensil_buffer_append_instruction(
        &driver->buffer, &driver->layout, TENSIL_OPCODE_DATA_MOVE,
        TENSIL_DATA_MOVE_FLAG_DRAM1_TO_LOCAL, SIMD_TEST_MULS_LOCAL_ADDRESS,
        SIMD_TEST_MULS_DRAM1_ADDRESS, SIMD_TEST_SIZE - 1);

    if (error)
        goto cleanup;

    error = tensil_buffer_append_instruction(
        &driver->buffer, &driver->layout, TENSIL_OPCODE_DATA_MOVE,
        TENSIL_DATA_MOVE_FLAG_DRAM1_TO_LOCAL, SIMD_TEST_ADDS_LOCAL_ADDRESS,
        SIMD_TEST_ADDS_DRAM1_ADDRESS, SIMD_TEST_SIZE - 1);

    if (error)
        goto cleanup;

    error = tensil_buffer_append_instruction(
        &driver->buffer, &driver->layout, TENSIL_OPCODE_DATA_MOVE,
        TENSIL_DATA_MOVE_FLAG_LOCAL_TO_ACC, SIMD_TEST_MULS_LOCAL_ADDRESS,
        SIMD_TEST_MULS_ACC_ADDRESS, SIMD_TEST_SIZE - 1);

    if (error)
        goto cleanup;

    error = tensil_buffer_append_instruction(
        &driver->buffer, &driver->layout, TENSIL_OPCODE_DATA_MOVE,
        TENSIL_DATA_MOVE_FLAG_LOCAL_TO_ACC, SIMD_TEST_ADDS_LOCAL_ADDRESS,
        SIMD_TEST_ADDS_ACC_ADDRESS, SIMD_TEST_SIZE - 1);

    if (error)
        goto cleanup;

    error = tensil_buffer_append_instruction(
        &driver->buffer, &driver->layout, TENSIL_OPCODE_DATA_MOVE,
        TENSIL_DATA_MOVE_FLAG_DRAM0_TO_LOCAL, SIMD_TEST_INPUT_LOCAL_ADDRESS,
        SIMD_TEST_INPUT_DRAM0_ADDRESS, SIMD_TEST_SIZE - 1);

    if (error)
        goto cleanup;

    error = tensil_buffer_append_instruction(
        &driver->buffer, &driver->layout, TENSIL_OPCODE_DATA_MOVE,
        TENSIL_DATA_MOVE_FLAG_LOCAL_TO_ACC, SIMD_TEST_INPUT_LOCAL_ADDRESS,
        SIMD_TEST_INPUT_ACC_ADDRESS, SIMD_TEST_SIZE - 1);

    if (error)
        goto cleanup;

    for (size_t i = 0; i < SIMD_TEST_SIZE; i++) {
        // TODO: need to respect SIMD op layout from arch
        // TODO: need to test SIMD ops other than Move, Multiply and Add
        // TODO: need to specialize the test when >1 SIMD registers are
        // available

        error = tensil_buffer_append_instruction(
            &driver->buffer, &driver->layout, TENSIL_OPCODE_SIMD,
            TENSIL_SIMD_FLAG_READ, 0, SIMD_TEST_INPUT_ACC_ADDRESS + i,
            (TENSIL_SIMD_OPCODE_MOVE << 3) | 0b001);

        if (error)
            goto cleanup;

        error = tensil_buffer_append_instruction(
            &driver->buffer, &driver->layout, TENSIL_OPCODE_SIMD,
            TENSIL_SIMD_FLAG_READ, 0, SIMD_TEST_MULS_ACC_ADDRESS + i,
            (TENSIL_SIMD_OPCODE_MUL << 3) | 0b101);

        if (error)
            goto cleanup;

        error = tensil_buffer_append_instruction(
            &driver->buffer, &driver->layout, TENSIL_OPCODE_SIMD,
            TENSIL_SIMD_FLAG_READ | TENSIL_SIMD_FLAG_WRITE,
            SIMD_TEST_OUTPUT_ACC_ADDRESS + i, SIMD_TEST_ADDS_ACC_ADDRESS + i,
            (TENSIL_SIMD_OPCODE_ADD << 3) | 0b100);

        if (error)
            goto cleanup;
    }

    error = tensil_buffer_append_instruction(
        &driver->buffer, &driver->layout, TENSIL_OPCODE_DATA_MOVE,
        TENSIL_DATA_MOVE_FLAG_ACC_TO_LOCAL, SIMD_TEST_OUTPUT_LOCAL_ADDRESS,
        SIMD_TEST_OUTPUT_ACC_ADDRESS, SIMD_TEST_SIZE - 1);

    if (error)
        goto cleanup;

    error = tensil_buffer_append_instruction(
        &driver->buffer, &driver->layout, TENSIL_OPCODE_DATA_MOVE,
        TENSIL_DATA_MOVE_FLAG_LOCAL_TO_DRAM0, SIMD_TEST_OUTPUT_LOCAL_ADDRESS,
        SIMD_TEST_OUTPUT_DRAM0_ADDRESS, SIMD_TEST_SIZE - 1);

    if (error)
        goto cleanup;

    error = tensil_driver_setup_buffer_postamble(driver);

    if (error)
        goto cleanup;

    error = tensil_driver_run(driver, NULL);

    if (error)
        goto cleanup;

    tensil_driver_read_dram_vectors(driver, TENSIL_DRAM0,
                                    SIMD_TEST_OUTPUT_DRAM0_ADDRESS, 0,
                                    SIMD_TEST_SIZE, to_buffer);

    for (size_t k = 0; k < SIMD_TEST_SIZE * driver->arch.array_size; k++) {
        from_buffer[k] = saturate(
            driver->arch.data_type,
            saturate(driver->arch.data_type, from_buffer[k] * SIMD_TEST_MUL) +
                SIMD_TEST_ADD);

        if (compare_scalars(driver->arch.data_type, from_buffer[k],
                            to_buffer[k])) {
            bad_indexes[bad_indexes_size++] = k;

            if (bad_indexes_size == TEST_MAX_BAD_INDEXES_SIZE)
                break;
        }
    }

    printf("%s\n", bad_indexes_size ? failed : ok);

    if (bad_indexes_size && verbose)
        for (size_t k = 0; k < bad_indexes_size; k++) {
            size_t bad_index = bad_indexes[k];

            printf("\t at %zu expected=%f, actual=%f\n", bad_index,
                   from_buffer[bad_index], to_buffer[bad_index]);
        }

cleanup:
    free(from_buffer);
    free(to_buffer);
    free(consts_buffer);

    return error;
}

#ifdef TENSIL_PLATFORM_SAMPLE_AXI_DMA_DEVICE_ID

#define SAMPLING_TEST_SIZE (64 * 1024 * 1024)

tensil_error_t tensil_driver_run_sampling_test(struct tensil_driver *driver,
                                               bool verbose) {
    tensil_error_t error = tensil_driver_setup_buffer_preamble(driver);

    if (error)
        return error;

    error = tensil_buffer_append_noop_instructions(
        &driver->buffer, &driver->layout, SAMPLING_TEST_SIZE);

    if (error)
        return error;

    error = tensil_driver_setup_buffer_postamble(driver);

    if (error)
        return error;

    error = tensil_driver_run(driver, NULL);

    if (error)
        return error;

    size_t samples_count =
        driver->sample_buffer.offset / TENSIL_SAMPLE_SIZE_BYTES;

    size_t valid_samples_count = 0;
    size_t valid_samples_base = 0;

    size_t stalling_samples_count = 0;
    size_t missing_samples_count = 0;

    const uint8_t *sample_ptr =
        tensil_sample_buffer_find_valid_samples_ptr(&driver->sample_buffer);

    uint32_t prev_program_counter = 0;
    uint32_t next_program_counter = 0;
    uint32_t instruction_offset = 0;

    while (tensil_sample_buffer_get_next_samples_ptr(
        &driver->sample_buffer, &driver->buffer, &driver->layout, &sample_ptr,
        &next_program_counter, &instruction_offset)) {
        valid_samples_count++;

        if (!prev_program_counter) {
            prev_program_counter = next_program_counter;
        } else {
            if (prev_program_counter == next_program_counter)
                stalling_samples_count++;
            else {
                if (next_program_counter >
                    prev_program_counter + TENSIL_SAMPLE_INTERVAL_CYCLES) {
                    if (verbose)
                        printf("Offset %u -> %u\n",
                               (unsigned int)prev_program_counter,
                               (unsigned int)next_program_counter);

                    missing_samples_count++;
                }
                prev_program_counter = next_program_counter;
            }
        }
    }

    printf("%s: collected %lu samples, %lu "
           "valid with %lu stalling and %lu missing, %lu head-invalid, %lu "
           "tail-invalid\n",
           missing_samples_count ? failed : ok, samples_count,
           valid_samples_count, stalling_samples_count, missing_samples_count,
           valid_samples_base,
           samples_count - valid_samples_base - valid_samples_count);

    return TENSIL_ERROR_NONE;
}

#endif

#endif
