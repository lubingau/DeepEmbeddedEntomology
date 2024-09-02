#ifndef CAMERA_H
#define CAMERA_H

#include "../ov5640/OV5640.h"
#include "../ov5640/ScuGicInterruptController.h"
#include "../ov5640/PS_GPIO.h"
#include "../ov5640/AXI_VDMA.h"
#include "../ov5640/PS_IIC.h"

#include "MIPI_D_PHY_RX.h"
#include "MIPI_CSI_2_RX.h"

#include "ff.h"

#include <xil_types.h>

/********************************************************************************************
 *
 * This program initializes the camera and the VDMA for the input pipeline, and allows to
 * capture images and save them to the SD card.
 *
 ********************************************************************************************/

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_ORANGE  "\x1b[33m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define IRPT_CTL_DEVID 		XPAR_PS7_SCUGIC_0_DEVICE_ID
#define GPIO_DEVID			XPAR_PS7_GPIO_0_DEVICE_ID
#define GPIO_IRPT_ID		XPAR_PS7_GPIO_0_INTR
#define CAM_I2C_DEVID		XPAR_PS7_I2C_0_DEVICE_ID
#define CAM_I2C_IRPT_ID		XPAR_PS7_I2C_0_INTR
#define VDMA_DEVID_0		XPAR_AXIVDMA_0_DEVICE_ID
#define VDMA_DEVID_1		XPAR_AXIVDMA_1_DEVICE_ID
#define VDMA_S2MM_IRPT_ID_0	XPAR_FABRIC_AXI_VDMA_0_S2MM_INTROUT_INTR
#define VDMA_S2MM_IRPT_ID_1	XPAR_FABRIC_AXI_VDMA_1_S2MM_INTROUT_INTR
#define CAM_I2C_SCLK_RATE	100000

#define DDR_BASE_ADDR		XPAR_DDR_MEM_BASEADDR
#define MEM_BASE_ADDR		(DDR_BASE_ADDR + 0x0A000000) // RGB image buffer
#define MEM_BASE_ADDR_1		(MEM_BASE_ADDR + 0x00600000) // Grayscale image buffer

#define GAMMA_BASE_ADDR     XPAR_AXI_GAMMACORRECTION_1_BASEADDR

#define FRAME_WIDTH	        1920
#define FRAME_HEIGHT	    1080
#define FRAME_TOTAL_PIXELS	(FRAME_WIDTH * FRAME_HEIGHT * 3)

using namespace digilent;

void pipeline_mode_change(AXI_VDMA<ScuGicInterruptController>& vdma_driver, OV5640& cam);
void init_second_vdma(AXI_VDMA<ScuGicInterruptController>& vdma_driver);
void define_resolution(int width, int height, int fps, OV5640_cfg::mode_t &mode);
void capture_image(AXI_VDMA<ScuGicInterruptController>& vdma_driver, u8 *image, int TOTAL_PIXELS);
void capture_mask(AXI_VDMA<ScuGicInterruptController>& vdma_driver, u8 *mask, int TOTAL_PIXELS);
void save_image_and_label(const u8 *image, int label, int TOTAL_PIXELS, const char *file_name);
void save_image(const u8 *image, int TOTAL_PIXELS, const char *file_name);
void save_crop(const u8 *image, int TOTAL_PIXELS, const char *file_name);
void delete_file(const char *file_name);

#endif /* CAMERA_H */
