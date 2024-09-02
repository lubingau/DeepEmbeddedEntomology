#include "camera.h"

#include <stdio.h>
#include <string.h>

/********************************************************************************************
 *
 * This program initializes the camera and the VDMA for the input pipeline, and allows to
 * capture images and save them to the SD card.
 *
 ********************************************************************************************/

using namespace digilent;

void pipeline_mode_change(AXI_VDMA<ScuGicInterruptController>& vdma_driver, OV5640& cam) //, OV5640_cfg::mode_t mode)
{	
	/*
		Initialize the camera and the VDMA for the input pipeline
	*/
	// Bring up input pipeline back-to-front
	vdma_driver.resetWrite();
	MIPI_CSI_2_RX_mWriteReg(XPAR_MIPI_CSI_2_RX_0_S_AXI_LITE_BASEADDR, CR_OFFSET, (CR_RESET_MASK & ~CR_ENABLE_MASK));
	MIPI_D_PHY_RX_mWriteReg(XPAR_MIPI_D_PHY_RX_0_S_AXI_LITE_BASEADDR, CR_OFFSET, (CR_RESET_MASK & ~CR_ENABLE_MASK));
	cam.reset();
	// VDMA configuration
	vdma_driver.configureWrite(1920, 1080);
	Xil_Out32(GAMMA_BASE_ADDR, 3); // Set Gamma correction factor to 1/1.8
	// Camera configuration
	cam.init();
	vdma_driver.enableWrite();
	MIPI_CSI_2_RX_mWriteReg(XPAR_MIPI_CSI_2_RX_0_S_AXI_LITE_BASEADDR, CR_OFFSET, CR_ENABLE_MASK);
	MIPI_D_PHY_RX_mWriteReg(XPAR_MIPI_D_PHY_RX_0_S_AXI_LITE_BASEADDR, CR_OFFSET, CR_ENABLE_MASK);
	cam.set_mode(OV5640_cfg::mode_t::MODE_1080P_1920_1080_30fps);
	cam.set_awb(OV5640_cfg::awb_t::AWB_ADVANCED);
	cam.set_isp_format(OV5640_cfg::isp_format_t::ISP_RAW);

	xil_printf("[SUCCESS] Pipeline configured\n\r");
}

void init_second_vdma(AXI_VDMA<ScuGicInterruptController>& vdma_driver)
{	
	/*
		Initialize the second VDMA for the mask pipeline
	*/
	// Write configuration
	vdma_driver.resetWrite();
	vdma_driver.configureWrite(1920, 1080);
	vdma_driver.enableWrite();

	xil_printf("[SUCCESS] Second VDMA configured\n\r");
}

void define_resolution(int width, int height, int fps, OV5640_cfg::mode_t &mode)
{	
	/*
		Define the resolution of the camera
	*/
	bool error = false;
	if (width == 1920 && height == 1080 && fps == 30) {
		mode = OV5640_cfg::mode_t::MODE_1080P_1920_1080_30fps;
	} else if (width == 1920 && height == 1080 && fps == 15) {
		mode = OV5640_cfg::mode_t::MODE_1080P_1920_1080_15fps;
	} else if (width == 1280 && height == 720 && fps == 60) {
		mode = OV5640_cfg::mode_t::MODE_720P_1280_720_60fps;
	} else if (fps == 0) {
		mode = OV5640_cfg::mode_t::MODE_END;
	} else {
		xil_printf(ANSI_COLOR_RED "[ERROR] Invalid resolution\n\r" ANSI_COLOR_RESET);
		error = true;
		goto ERROR;
	}
	xil_printf("[SUCCESS] Resolution defined\n\r");

ERROR:
	if (error) {xil_printf(ANSI_COLOR_RED "[ERROR] Failed to define the resolution\n\r" ANSI_COLOR_RESET);}
}

void capture_image(AXI_VDMA<ScuGicInterruptController>& vdma_driver, u8 *image, int TOTAL_PIXELS)
{	
	/*
		Capture an image from the camera
	*/
	// Stop writing to the frame buffer
	vdma_driver.stopWrite();
	// Copy the frame buffer to the image buffer
	memcpy(image, (u8 *)MEM_BASE_ADDR-1, TOTAL_PIXELS);
	// Enable writing to the frame buffer
	vdma_driver.startWrite();

	xil_printf("[SUCCESS] Image captured\n\r");
}

void capture_mask(AXI_VDMA<ScuGicInterruptController>& vdma_driver, u8 *mask, int TOTAL_PIXELS)
{	
	/*
		Capture a grayscale mask from the camera
	*/
	// Stop writing to the frame buffer
	vdma_driver.stopWrite();
	// Copy the frame buffer to the image buffer
	memcpy(mask, (u8 *)MEM_BASE_ADDR_1, TOTAL_PIXELS);
	// Enable writing to the frame buffer
	vdma_driver.startWrite();

	xil_printf("[SUCCESS] Mask captured\n\r");
}

void save_image_and_label(const u8 *image, int label, int TOTAL_PIXELS, const char *file_name)
{	
	/*
		Save an image and its label to the SD card
	*/
	bool error = false;
	// SD card setup
	FRESULT res;
	FIL fil;
	FILINFO fno;
	UINT bytes_read;
	// Get the info of the file (like the size)
	res = f_stat(file_name, &fno);
	if (res != FR_OK) {
		if (res == FR_NO_FILE) {
			xil_printf(ANSI_COLOR_ORANGE "[WARNING] File %s not found. File will be created\n\r" ANSI_COLOR_RESET, file_name);
		} else if ((res == FR_NOT_READY) || (res == FR_DISK_ERR)) {
			xil_printf(ANSI_COLOR_RED "[ERROR] No disk found. Please insert an SD card\n\r" ANSI_COLOR_RESET);
			error = true;
			goto ERROR;
		} else {
			xil_printf(ANSI_COLOR_RED "[ERROR] Error getting file info: %d\n\r" ANSI_COLOR_RESET, res);
			error = true;
			goto ERROR;
		}
	} else {
		xil_printf("[INFO] File found: %s, size: %d\n\r", file_name, fno.fsize);
	}

	// Open the file
	res = f_open(&fil, file_name, FA_CREATE_ALWAYS | FA_WRITE);
	if (res != FR_OK) {
		xil_printf(ANSI_COLOR_RED "[ERROR] Error opening file: %d\n\r" ANSI_COLOR_RESET, res);
		error = true;
		goto ERROR;
	}
	// Write the class on 16 bits
	f_write(&fil, &label, sizeof(u16), &bytes_read);
	// Write the image
	f_write(&fil, image, TOTAL_PIXELS, &bytes_read);

	xil_printf("[SUCCESS] Image and label written to SD card as: %s\n\r", file_name);

ERROR:
	if (error) {xil_printf(ANSI_COLOR_RED "[ERROR] Failed to write the image to the SD card\n\r" ANSI_COLOR_RESET);}

	f_sync(&fil);
	f_close(&fil);
}

void save_image(const u8 *image, int TOTAL_PIXELS, const char *file_name)
{	
	/*
		Save an image to the SD card
	*/
	bool error = false;
	// SD card setup
	FRESULT res;
	FIL fil;
	FILINFO fno;
	UINT bytes_read;
	// Get the info of the file (like the size)
	res = f_stat(file_name, &fno);
	if (res != FR_OK) {
		if (res == FR_NO_FILE) {
			xil_printf(ANSI_COLOR_ORANGE "[WARNING] File %s not found. File will be created\n\r" ANSI_COLOR_RESET, file_name);
		} else if ((res == FR_NOT_READY) || (res == FR_DISK_ERR)) {
			xil_printf(ANSI_COLOR_RED "[ERROR] No disk found. Please insert an SD card\n\r" ANSI_COLOR_RESET);
			error = true;
			goto ERROR;
		} else {
			xil_printf(ANSI_COLOR_RED "[ERROR] Error getting file info: %d\n\r" ANSI_COLOR_RESET, res);
			error = true;
			goto ERROR;
		}
	} else {
		xil_printf("[INFO] File found: %s, size: %d\n\r", file_name, fno.fsize);
	}
	
	// Open the file
	res = f_open(&fil, file_name, FA_CREATE_ALWAYS | FA_WRITE);
	if (res != FR_OK) {
		xil_printf(ANSI_COLOR_RED "[ERROR] Error opening file: %d\n\r" ANSI_COLOR_RESET, res);
		error = true;
		goto ERROR;
	}

	// Write the image
	f_write(&fil, image, TOTAL_PIXELS, &bytes_read);

	xil_printf("[SUCCESS] Image written to SD card as: %s\n\r", file_name);

ERROR:
	if (error) {xil_printf(ANSI_COLOR_RED "[ERROR] Failed to write the image to the SD card\n\r" ANSI_COLOR_RESET);}

	f_sync(&fil);
	f_close(&fil);
}


void save_crop(const u8 *image, int TOTAL_PIXELS, const char *file_name)
{	
	/*
		Save a cropped image to the SD card
	*/
	bool error = false;
	// SD card setup
	FRESULT res;
	FIL fil;
	FILINFO fno;
	UINT bytes_read;

	FATFS fatfs;
	res = f_mount(&fatfs, "0:", 0);
	if (res != FR_OK) {
		xil_printf(ANSI_COLOR_RED "[ERROR] Error mounting SD card: %d\n\r" ANSI_COLOR_RESET, res);
		goto ERROR;
	}
//	xil_printf("[INFO] SD card mounted\n\r");

	// Get the info of the file (like the size)
	res = f_stat(file_name, &fno);
	if (res != FR_OK) {
		if (res == FR_NO_FILE) {
			xil_printf(ANSI_COLOR_ORANGE "[WARNING] File %s not found. File will be created\n\r" ANSI_COLOR_RESET, file_name);
		} else if ((res == FR_NOT_READY) || (res == FR_DISK_ERR)) {
			xil_printf(ANSI_COLOR_RED "[ERROR] No disk found. Please insert an SD card\n\r" ANSI_COLOR_RESET);
			error = true;
			goto ERROR;
		} else {
			xil_printf(ANSI_COLOR_RED "[ERROR] Error getting file info: %d\n\r" ANSI_COLOR_RESET, res);
			error = true;
			goto ERROR;
		}
	} else {
		xil_printf("[INFO] File found: %s, size: %d\n\r", file_name, fno.fsize);
	}

	// Open the file
	res = f_open(&fil, file_name, FA_OPEN_APPEND | FA_WRITE);
	if (res != FR_OK) {
		xil_printf(ANSI_COLOR_RED "[ERROR] Error opening file: %d\n\r" ANSI_COLOR_RESET, res);
		error = true;
		goto ERROR;
	}
	// Write the image
	f_write(&fil, image, TOTAL_PIXELS, &bytes_read);

	xil_printf("[SUCCESS] Image written to SD card as: %s\n\r", file_name);

ERROR:
	if (error) {xil_printf(ANSI_COLOR_RED "[ERROR] Failed to write the image to the SD card\n\r" ANSI_COLOR_RESET);}

	f_sync(&fil);
	f_close(&fil);
	f_unmount("0:");
}


