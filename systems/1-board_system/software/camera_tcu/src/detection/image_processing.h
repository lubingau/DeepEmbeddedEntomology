#include "xil_printf.h"

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <cstdint>

using namespace std;

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_ORANGE  "\x1b[33m"
#define ANSI_COLOR_RESET   "\x1b[0m"

/********************************************************************************************
 *
 * This program contains functions to process images, such as cropping, resizing, and color
 * correction.
 *
 ********************************************************************************************/
;
void resize_image(const u8 *image, u8 *resized_image, int image_width, int image_height, int new_width, int new_height);
void resize_mask(const u8 *image, u8 *resized_image, int image_width, int image_height, int new_width, int new_height);
void crop_image(const u8 *image, u8 *crop, int image_width, int image_height, int x0, int y0, int x1, int y1);
void applyAWB(u8* image, int width, int height);
void applyBrightnessContrast(u8* image, int width, int height, int brightness, double contrast);
void color_correction(u8* image, int width, int height);
