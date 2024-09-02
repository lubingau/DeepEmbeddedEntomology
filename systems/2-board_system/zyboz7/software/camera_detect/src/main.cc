#include <malloc.h>
#include <stdio.h>
#include <unistd.h>

#include "xparameters.h"
#include "xgpio.h"

#include "platform/platform.h"
#include "console/console.h"

#include "console/stopwatch.h"
#include "detection/camera.h"
#include "detection/image_processing.h"
#include "detection/detect.h"
#include "detection/private_timer_int.h"
#include "model/dataset.h"
#include "uart/xuartlite_intr.h"

using namespace digilent;

/********************************************************************************************
 *
 * This program implements the insect detection system.
 * The system captures images from the camera, detects insects, crops the insects on the image,
 * classifies the insect with a CNN model, and displays the results on the console.
 *
 ********************************************************************************************/


/*************************** Constant Definitions ***************************************/
#define DATASET_BUFFER_BASE TENSIL_PLATFORM_DRAM_BUFFER_HIGH

u8 image[FRAME_TOTAL_PIXELS/3];
u8 imageRGB[FRAME_TOTAL_PIXELS];
u8 background_image[FRAME_TOTAL_PIXELS/3];
u8 resized_image[FRAME_TOTAL_PIXELS];
u8 crop[FRAME_TOTAL_PIXELS];
float image_float_1[FRAME_TOTAL_PIXELS];


/********************************************************************************************
 *
 * Main function
 *
 ********************************************************************************************/

int main()
{
	init_platform();
	console_reset_foreground_color();
	xil_printf("\r\nSTART\r\n");

	// Initialize the camera, GPIO, I2C, VDMA, and interrupt controller
	XGpio gpio;
	XGpio_Initialize(&gpio, XPAR_AXI_GPIO_0_DEVICE_ID);
	ScuGicInterruptController irpt_ctl(IRPT_CTL_DEVID);
	PS_GPIO<ScuGicInterruptController> gpio_driver(GPIO_DEVID, irpt_ctl, GPIO_IRPT_ID);
	PS_IIC<ScuGicInterruptController> iic_driver(CAM_I2C_DEVID, irpt_ctl, CAM_I2C_IRPT_ID, 100000);

	OV5640 cam(iic_driver, gpio_driver);
	// OV5640_cfg::mode_t mode = OV5640_cfg::mode_t::MODE_1080P_1920_1080_30fps;
	AXI_VDMA<ScuGicInterruptController> vdma_driver0(VDMA_DEVID_0, MEM_BASE_ADDR, irpt_ctl, VDMA_S2MM_IRPT_ID_0);
	AXI_VDMA<ScuGicInterruptController> vdma_driver1(VDMA_DEVID_1, MEM_BASE_ADDR_1, irpt_ctl, VDMA_S2MM_IRPT_ID_1);

	int console_height = 44;
	int console_width = console_height * 16 / 9;

	//Image detection main arrays initialization
	u16 locations[FRAME_WIDTH * FRAME_HEIGHT * 2];
	int num_locations;
	int unique_mass_centers[MAX_CLUSTERS][2] = {0};
	int grouped_clusters[MAX_CLUSTERS][MAX_LOCATIONS][2] = {0};
	int min_max_coordinates[MAX_CLUSTERS][4] = {0};
	int dimensions[MAX_CLUSTERS][2] = {0};
	u8 sizing_factor = 4;

	// First pipeline configuration
	pipeline_mode_change(vdma_driver0, cam);
	// Second pipeline configuration
	init_second_vdma(vdma_driver1);

	// define_resolution(IMAGE_WIDTH, IMAGE_HEIGHT, FPS, mode);

	// Mount the SD card
	FATFS fatfs;
	FRESULT res = f_mount(&fatfs, "0:", 0);
	if (res != FR_OK) {
		xil_printf(ANSI_COLOR_RED "[ERROR] Error mounting SD card: %d\n\r" ANSI_COLOR_RESET, res);
		xil_printf("[ERROR] %d \r\n", res);
		return 0;
	}
	xil_printf("[INFO] SD card mounted\n\r");
	delete_file("crops.bin");

	// Configure Private Timer for Interruptions
	int Status = ScuTimerIntrConfig(&IntcInstance, &TimerInstance, TIMER_DEVICE_ID, TIMER_IRPT_INTR);
	if (Status != XST_SUCCESS) {
		xil_printf("[ERROR] Private Timer Interrupt Failed on Configuration \r\n");
		return XST_FAILURE;
	}
	xil_printf("[INFO] Private Timer Interrupt Success on Configuration \r\n");

	// Configure UartLite
	Status = XUartLite_Initialization(&IntcInstanceUart, &UartLite, UARTLITE_DEVICE_ID, UARTLITE_IRPT_INTR);
 	if (Status != XST_SUCCESS) {
 		xil_printf("UartLite initialization failed\r\n");
 		return XST_FAILURE;
 	}
 	xil_printf("[INFO] UartLite initialized\r\n");

	// Load Buffer to send
	for (int i = 0; i < BUFFER_SIZE; i++) {
 		if (i%3==0) {
 			Buffer[i] = 255;
 		} else {
 			Buffer[i] = 0;
 		}
	}

	// MAIN LOOP
	while (1) {

		// Read from BTN
		u32 btn = XGpio_DiscreteRead(&gpio, 1);
		// Put on LEDs
		XGpio_DiscreteWrite(&gpio, 2, btn);

		if (btn == 1) {
			capture_image(vdma_driver0, imageRGB, FRAME_TOTAL_PIXELS);
			color_correction(imageRGB, FRAME_WIDTH, FRAME_HEIGHT);
			image_on_console(imageRGB, FRAME_WIDTH, FRAME_HEIGHT);
			resize_image(imageRGB, resized_image, FRAME_WIDTH, FRAME_HEIGHT, IMAGE_WIDTH, IMAGE_HEIGHT);
			save_crop(resized_image, IMAGE_TOTAL_PIXELS, "crops.bin");
			usleep(500000);
		}

		if (btn == 2) {
			xil_printf("[DEBUG] Send resized image\r\n");
			XUartLite_Send(&UartLite, resized_image, IMAGE_TOTAL_PIXELS);
			xil_printf("\r\nUART started to send %d bytes\r\n", IMAGE_TOTAL_PIXELS);

			while (TotalSentCount != IMAGE_TOTAL_PIXELS) {
			}
			TotalSentCount = 0;
			xil_printf("UART send finished\r\n");
			usleep(500000);
		}
		
		if (btn == 4) {
			sendImagesToHost("crops.bin");

			usleep(500000);
		}


		// if (btn == 1) {
		// 	capture_mask(vdma_driver1, background_image, FRAME_TOTAL_PIXELS/3);
		// 	resize_mask(background_image, resized_image, FRAME_WIDTH, FRAME_HEIGHT, console_width, console_height);
		// 	mask_on_console(resized_image, console_width, console_height);
		// 	sleep(1);
		// 	XScuTimer_Start(&TimerInstance); //Private Timer
		// }

		// if (btn == 2) {
		// 	xil_printf("[DEBUG] resized_image: \r\n");
		// 	save_crop(resized_image, IMAGE_TOTAL_PIXELS, "ImageDoc12.bin");
		// 	image_on_console(resized_image, IMAGE_WIDTH, IMAGE_HEIGHT);
		// 	usleep(500000);
		// }


		// if (btn == 4) {
		// 	XScuTimer_Stop(&TimerInstance); //Private Timer
		// 	xil_printf("\n\r[INFO] Interruption stopped\n\r");
		// 	usleep(500000);
		// }

		if (TIMER_FLAG == true){
			capture_image(vdma_driver0, imageRGB, FRAME_TOTAL_PIXELS); //Capture RGB Image
			capture_mask(vdma_driver1, image, FRAME_TOTAL_PIXELS/3);	//Capture Grayscale Image
			resize_image(imageRGB, resized_image, FRAME_WIDTH, FRAME_HEIGHT, console_width, console_height);
			image_on_console(resized_image, console_width, console_height);

			runDetection(image, background_image, imageRGB, crop, unique_mass_centers, min_max_coordinates,
					grouped_clusters, dimensions, locations, &num_locations, sizing_factor);
			xil_printf("\n\r[SUCCESS] Detection done\n\r");

			// Print Clusters if Minimum and Maximum dimensions> 224 x 224, else using MassCenter
			// printClusterCrops(imageRGB, crop, unique_mass_centers, min_max_coordinates, resized_image,
			// 		dimensions, console_height, console_width, sizing_factor);

			// Print Clusters with Minimum and Maximum locations only
			// MinMaxCrop(imageRGB, crop, unique_mass_centers, min_max_coordinates, resized_image,
			// 					dimensions, console_height, console_width, sizing_factor);

			// Print Clusters with MassCenter
			MassCenterCrop(imageRGB, crop, unique_mass_centers, min_max_coordinates, resized_image,
								dimensions, console_height, console_width, sizing_factor);

			xil_printf("\n\r[SUCCESS] Crop saved\n\r");

			TIMER_FLAG = false;
			usleep(500000);
		}

	}

	cleanup_platform();

	return 0;
}
