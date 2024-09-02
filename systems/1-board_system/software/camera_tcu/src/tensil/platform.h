/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright © 2019-2022 Tensil AI Company */

#pragma once

#define TENSIL_PLATFORM_DECODER_TIMEOUT 100
#define TENSIL_PLATFORM_SAMPLE_BLOCK_SIZE 1024

#define TENSIL_PLATFORM_ENABLE_FILE_SYSTEM
#define TENSIL_PLATFORM_ENABLE_STDIO

#define TENSIL_PLATFORM_INSTRUCTION_AXI_DMA_DEVICE_ID XPAR_AXIDMA_0_DEVICE_ID

#define TENSIL_PLATFORM_PROG_BUFFER_BASE 0x0b000000 //0x00400000  ->  must be higher than 0x0a000000 (frame buffer)
#define TENSIL_PLATFORM_PROG_BUFFER_HIGH (TENSIL_PLATFORM_PROG_BUFFER_BASE + 0x10000000) // 0x08000000

#define TENSIL_PLATFORM_DRAM_BUFFER_BASE TENSIL_PLATFORM_PROG_BUFFER_HIGH
#define TENSIL_PLATFORM_DRAM_BUFFER_HIGH (TENSIL_PLATFORM_PROG_BUFFER_HIGH + 0x08000000) // 0x0c000000

#define TENSIL_PLATFORM_SAMPLE_BUFFER_BASE TENSIL_PLATFORM_DRAM_BUFFER_HIGH
#define TENSIL_PLATFORM_SAMPLE_BUFFER_HIGH (TENSIL_PLATFORM_DRAM_BUFFER_HIGH + 0x03000000) // 0x0f000000
