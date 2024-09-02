#include "image_processing.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

/********************************************************************************************
 *
 * This program contains functions to process images, such as cropping, resizing, and color
 * correction.
 *
 ********************************************************************************************/

void crop_image(const u8 *image, u8 *crop, int image_width, int image_height, int x0, int y0, int x1, int y1)
{	
	/*
		Crop the image to the specified locations
	*/
	bool error = false;
    int new_width = x1 - x0; // new width
    int new_height = y1 - y0; // new height
    if (new_width > image_width || new_height > image_height) {
        xil_printf(ANSI_COLOR_RED "[ERROR] Invalid crop dimensions\n\r" ANSI_COLOR_RESET);
		error = true;
		goto ERROR;
    }
    for (int y = 0; y < new_height; y++) {
        memcpy(crop + y * new_width * 3, image + (y + y0) * image_width * 3 + x0 * 3, new_width * 3);
    }
	xil_printf("[SUCCESS] Image cropped\n\r");

ERROR:
	if (error) {xil_printf(ANSI_COLOR_RED "[ERROR] Failed to crop the image\n\r" ANSI_COLOR_RESET);}
}

void resize_image(const u8 *image, u8 *resized_image, int image_width, int image_height, int new_width, int new_height)
{	
	/*
		Resize the image to the specified dimensions. The image is resized using the nearest.
	*/
	bool error = false;
	if (new_width > image_width || new_height > image_height) {
		xil_printf(ANSI_COLOR_RED "[ERROR] Invalid resize dimensions\n\r" ANSI_COLOR_RESET);
		xil_printf(ANSI_COLOR_RED "[ERROR] Input size: %dx%d. New size: %dx%d\n\r" ANSI_COLOR_RESET, image_width, image_height, new_width, new_height);
		error = true;
		goto ERROR;
	}
	for (int y = 0; y < new_height; y++) {
		for (int x = 0; x < new_width; x++) {
			int x0 = x * image_width / new_width;
			int y0 = y * image_height / new_height;
			if ((x == 0) || (y == 0)) { // Correct the gray border line introduce by the digilent axis_raw_demosaic IP (cf Pcam demo)
				resized_image[(y * new_width + x) * 3] = image[((y0+1) * image_width + x0) * 3 + 3];
				resized_image[(y * new_width + x) * 3 + 1] = image[((y0+1) * image_width + x0) * 3 + 1 + 3];
				resized_image[(y * new_width + x) * 3 + 2] = image[((y0+1) * image_width + x0) * 3 + 2 + 3];
			} else {
				resized_image[(y * new_width + x) * 3] = image[(y0 * image_width + x0) * 3];
				resized_image[(y * new_width + x) * 3 + 1] = image[(y0 * image_width + x0) * 3 + 1];
				resized_image[(y * new_width + x) * 3 + 2] = image[(y0 * image_width + x0) * 3 + 2];
			}
		}
	}
	xil_printf("[SUCCESS] Image resized\n\r");

ERROR:
	if (error) {xil_printf(ANSI_COLOR_RED "[ERROR] Failed to resize the image\n\r" ANSI_COLOR_RESET);}
}

void resize_mask(const u8 *image, u8 *resized_image, int image_width, int image_height, int new_width, int new_height)
{	
	/*
		Resize the mask to the specified dimensions. The mask is resized using the nearest.
	*/
	bool error = false;
	if (new_width > image_width || new_height > image_height) {
		xil_printf(ANSI_COLOR_RED "[ERROR] Invalid resize dimensions\n\r" ANSI_COLOR_RESET);
		error = true;
		goto ERROR;
	}
	for (int y = 0; y < new_height; y++) {
		for (int x = 0; x < new_width; x++) {
			int x0 = x * image_width / new_width;
			int y0 = y * image_height / new_height;
			if ((x == 0) || (y == 0)) { // Correct the gray border line introduce by the digilent axis_raw_demosaic IP (cf Pcam demo)
				resized_image[y * new_width + x] = image[(y0+1) * image_width + x0 + 3];
			} else {
				resized_image[y * new_width + x] = image[y0 * image_width + x0];
			}
		}
	}
	xil_printf("[SUCCESS] Image resized\n\r");

ERROR:
	if (error) {xil_printf(ANSI_COLOR_RED "[ERROR] Failed to resize the image\n\r" ANSI_COLOR_RESET);}
}

void applyAWB(u8* image, int width, int height) {
	/*
		Apply automatic white balance to the image
	*/
    // Calculate the average of each channel
    double sumB = 0, sumG = 0, sumR = 0;
    int total_pixel = width * height * 3;

    for (int i = 0; i < total_pixel; i += 3) {
        sumB += image[i];
        sumG += image[i + 1];
        sumR += image[i + 2];
    }

    double avgB = sumB / (total_pixel/3);
    double avgG = sumG / (total_pixel/3);
    double avgR = sumR / (total_pixel/3);
    double avgGray = (avgB + avgG + avgR) / 3.0;

    double scaleB = avgGray / avgB;
    double scaleG = avgGray / avgG;
    double scaleR = avgGray / avgR;

    // Apply scale factors to each channel
    for (int i = 0; i < total_pixel; i += 3) {
        image[i] = min(255.0, image[i] * scaleB);
        image[i + 1] = min(255.0, image[i + 1] * scaleG);
        image[i + 2] = min(255.0, image[i + 2] * scaleR);
    }
}

void applyBrightnessContrast(u8* image, int width, int height, int brightness, double contrast) {
	/*
		Apply brightness and contrast to the image
	*/
    int total_pixel = width * height * 3;
    for (int i = 0; i < total_pixel; i++) {
        int value = (int)(image[i] * contrast + brightness);
        image[i] = min(255, max(0, value));
    }
}

void color_correction(u8* image, int width, int height) {
	/*
		Apply color correction to the image by adjusting the white balance, brightness, and contrast
	*/
    applyAWB(image, width, height);
    int brightness = -35;
    double contrast = 1.25;
    applyBrightnessContrast(image, width, height, brightness, contrast);
}
