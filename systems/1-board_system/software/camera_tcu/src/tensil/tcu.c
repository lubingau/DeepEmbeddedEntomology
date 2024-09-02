/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright © 2019-2022 Tensil AI Company */

#include "tcu.h"

#include "instruction_buffer.h"
#include "platform.h"
#include "sample_buffer.h"

#if defined(TENSIL_PLATFORM_INSTRUCTION_AXI_DMA_DEVICE_ID) ||                  \
    defined(TENSIL_PLATFORM_SAMPLE_AXI_DMA_DEVICE_ID)

static tensil_error_t init_axi_dma(uint32_t axi_dma_device_id,
                                   XAxiDma *axi_dma) {
    XAxiDma_Config *config;
    int status;

    config = XAxiDma_LookupConfig(axi_dma_device_id);
    if (!config)
        return TENSIL_DRIVER_ERROR(TENSIL_ERROR_DRIVER_AXI_DMA_DEVICE_NOT_FOUND,
                                   "AXI DMA device %d not found",
                                   axi_dma_device_id);

    status = XAxiDma_CfgInitialize(axi_dma, config);
    if (status != XST_SUCCESS)
        return TENSIL_XILINX_ERROR(status);

    status = XAxiDma_Selftest(axi_dma);
    if (status != XST_SUCCESS)
        return TENSIL_XILINX_ERROR(status);

    XAxiDma_IntrDisable(axi_dma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DMA_TO_DEVICE);

    XAxiDma_IntrDisable(axi_dma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);

    return TENSIL_ERROR_NONE;
}

#endif

#ifdef TENSIL_PLATFORM_SAMPLE_AXI_DMA_DEVICE_ID
tensil_error_t
tensil_compute_unit_init_sampling(struct tensil_compute_unit *tcu,
                                  size_t sample_block_size) {
    tcu->sample_block_size = sample_block_size;

    tensil_error_t error = init_axi_dma(
        TENSIL_PLATFORM_SAMPLE_AXI_DMA_DEVICE_ID, &tcu->sample_axi_dma);

    if (error)
        return error;

    return TENSIL_ERROR_NONE;
}
#endif

#ifdef TENSIL_PLATFORM_INSTRUCTION_AXI_DMA_DEVICE_ID

tensil_error_t tensil_compute_unit_init(struct tensil_compute_unit *tcu) {
    tensil_error_t error =
        init_axi_dma(TENSIL_PLATFORM_INSTRUCTION_AXI_DMA_DEVICE_ID,
                     &tcu->instruction_axi_dma);

    if (error)
        return error;

    return TENSIL_ERROR_NONE;
}

tensil_error_t tensil_compute_unit_start_instructions(
    struct tensil_compute_unit *tcu,
    const struct tensil_instruction_buffer *buffer, size_t *run_offset) {
    const uint8_t *transfer_ptr = buffer->ptr + *run_offset;
    size_t transfer_size = buffer->offset - *run_offset;

    if (transfer_size > tcu->instruction_axi_dma.TxBdRing.MaxTransferLen)
        transfer_size = tcu->instruction_axi_dma.TxBdRing.MaxTransferLen;

    transfer_size &= ~((tcu->instruction_axi_dma.TxBdRing.DataWidth) - 1);

    (*run_offset) += transfer_size;

    int status;
    status =
        XAxiDma_SimpleTransfer(&tcu->instruction_axi_dma, (UINTPTR)transfer_ptr,
                               transfer_size, XAXIDMA_DMA_TO_DEVICE);

    if (status != XST_SUCCESS)
        return TENSIL_XILINX_ERROR(status);

    return TENSIL_ERROR_NONE;
}

bool tensil_compute_unit_is_instructions_busy(struct tensil_compute_unit *tcu) {
    return XAxiDma_Busy(&tcu->instruction_axi_dma, XAXIDMA_DMA_TO_DEVICE);
}

int tensil_compute_unit_get_instructions_data_width_bytes(
    struct tensil_compute_unit *tcu) {
    return tcu->instruction_axi_dma.TxBdRing.DataWidth;
}

#endif

#ifdef TENSIL_PLATFORM_SAMPLE_AXI_DMA_DEVICE_ID

tensil_error_t
tensil_compute_unit_start_sampling(struct tensil_compute_unit *tcu,
                                   struct tensil_sample_buffer *buffer) {
    uint8_t *transfer_ptr = buffer->ptr + buffer->offset;
    size_t transfer_size = tcu->sample_block_size * TENSIL_SAMPLE_SIZE_BYTES;

    if (transfer_size > buffer->size - buffer->offset)
        return TENSIL_DRIVER_ERROR(TENSIL_ERROR_DRIVER_OUT_OF_SAMPLE_BUFFER,
                                   "Out of sample buffer");

    int status;

    status = XAxiDma_SimpleTransfer(&tcu->sample_axi_dma, (UINTPTR)transfer_ptr,
                                    transfer_size, XAXIDMA_DEVICE_TO_DMA);

    if (status != XST_SUCCESS)
        return TENSIL_XILINX_ERROR(status);

    return TENSIL_ERROR_NONE;
}

void tensil_compute_unit_complete_sampling(
    struct tensil_compute_unit *tcu, struct tensil_sample_buffer *buffer) {
    size_t transfered_size = XAxiDma_ReadReg(
        tcu->sample_axi_dma.RxBdRing[0].ChanBase, XAXIDMA_BUFFLEN_OFFSET);

    buffer->offset += transfered_size;
}

bool tensil_compute_unit_is_sample_busy(struct tensil_compute_unit *tcu) {
    return XAxiDma_Busy(&tcu->sample_axi_dma, XAXIDMA_DEVICE_TO_DMA);
}

#endif
