/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright � 2019-2022 Tensil AI Company */

#include "console.h"
#include "../detection/image_processing.h"

#include <stdio.h>
#include <string.h>
#include <xil_io.h>

void console_set_cursor_position(int x, int y) {
    printf("\033[%d;%dH", x, y);
    fflush(stdout);
}

void console_clear_screen() {
    printf("\033[2J");
    fflush(stdout);
}

void console_set_foreground_color(int r, int g, int b) {
    printf("\033[38;2;%d;%d;%dm", r, g, b);
    fflush(stdout);
}

void console_reset_foreground_color() {
    printf("\033[39m");
    fflush(stdout);
}

void console_set_background_color(int r, int g, int b) {
    printf("\033[48;2;%d;%d;%dm", r, g, b);
    fflush(stdout);
}

void console_reset_background_color() {
    printf("\033[49m");
    fflush(stdout);
}

#define ESC_BUFFER_SIZE 128

bool console_get_cursor_position(int *x, int *y) {
    printf("\033[6n");
    fflush(stdout);

    char b = inbyte();

    if (b == '\033') {
        char buff[ESC_BUFFER_SIZE];
        memset(buff, 0, ESC_BUFFER_SIZE);

        for (size_t i = 0; i < ESC_BUFFER_SIZE; i++) {
            buff[i] = b;

            if (b == 'R')
                break;

            b = inbyte();
        }

        sscanf(buff, "\033[%d;%dR", x, y);
        return true;
    }

    return false;
}

//void image_on_console(const u8 *image, int image_width, int image_height)
//{
//	int TOTAL_PIXELS = image_width * image_height * 3;
//    u8 tmp_image[TOTAL_PIXELS];
//	if (image_width > 32 || image_height > 32) {
//		xil_printf(ANSI_COLOR_ORANGE "[WARNING] Image too big to be displayed (%dx%d). Image will be resized to 32x32\n\r" ANSI_COLOR_RESET, image_width, image_height);
//        resize_image(image, tmp_image, image_width, image_height, 32, 32);
//        image_width = 32;
//        image_height = 32;
//        TOTAL_PIXELS = image_width * image_height * 3;
//	} else {
//        memcpy(tmp_image, image, TOTAL_PIXELS);
//    }
//	// // Clear the console
//	// console_clear_screen();
//	// console_set_cursor_position(1, 1);
//
//	for (int i = 0; i < TOTAL_PIXELS; i += 3) {
//		console_set_background_color(tmp_image[i], tmp_image[i + 1], tmp_image[i + 2]);
//		if ((i % (image_width * 3) == 0)) {
//			printf("\r\n");
//		}
//		printf("  ");
//	}
//	printf("\r\n");
//    console_reset_background_color();
//
//	// printf("\n\r[SUCCESS] Image displayed. Size: %dx%d\n\r", image_width, image_height);
//}

void image_on_console(const u8 *image, int image_width, int image_height)
{   
    /*
        Display the RGB image on the console by setting the background color of the console to the color of the pixel.
    */
	int TOTAL_PIXELS = image_width * image_height * 3;
    u8 tmp_image[TOTAL_PIXELS];
	if (image_width > 32 || image_height > 32) {
		xil_printf(ANSI_COLOR_ORANGE "[WARNING] Image too big to be displayed. Image will be resized to 32x32\n\r" ANSI_COLOR_RESET);
        resize_image(image, tmp_image, image_width, image_height, 32, 32);
        image_width = 32;
        image_height = 32;
        TOTAL_PIXELS = image_width * image_height * 3;
	} else {
        memcpy(tmp_image, image, TOTAL_PIXELS);
    }
	// Clear the console
	console_clear_screen(); // Comment these 2 lines for evaluation
	console_set_cursor_position(1, 1);

	for (int i = 0; i < TOTAL_PIXELS; i += 3) {
		console_set_background_color(tmp_image[i], tmp_image[i + 1], tmp_image[i + 2]);
		if ((i % (image_width * 3) == 0)) {
			printf("\r\n");
		}
		printf("  ");
	}
	printf("\r\n");
	console_reset_background_color();
	
	// printf("\n\r[SUCCESS] Image displayed. Size: %dx%d\n\r", image_width, image_height);
}

void mask_on_console(const u8 *image, int image_width, int image_height)
{   
    /*
        Display the grayscale mask on the console by setting the background color of the console to the color of the pixel.
    */
	int TOTAL_PIXELS = image_width * image_height;
    u8 tmp_image[TOTAL_PIXELS];
	if (image_width > 32 || image_height > 32) {
		xil_printf(ANSI_COLOR_ORANGE "[WARNING] Image too big to be displayed. Image will be resized to 32x32\n\r" ANSI_COLOR_RESET);
        resize_mask(image, tmp_image, image_width, image_height, 32, 32);
        image_width = 32;
        image_height = 32;
        TOTAL_PIXELS = image_width * image_height;
	} else {
        memcpy(tmp_image, image, TOTAL_PIXELS);
    }
	// Clear the console
	console_clear_screen();
	console_set_cursor_position(1, 1);

	for (int i = 0; i < TOTAL_PIXELS; i ++) {
		console_set_background_color(tmp_image[i], tmp_image[i], tmp_image[i]);
		if ((i % (image_width) == 0)) {
			printf("\r\n");
		}
		printf("  ");
	}
	printf("\r\n");
	console_reset_background_color();

	printf("\n\r[SUCCESS] Image displayed. Size: %dx%d\n\r", image_width, image_height);
}

void printAccuracyBars(const float accuracies[], const char* labels[], int n_classes) {
    /*
        Print the accuracy of each class as a bar in the console.
    */
    const char barChar = '#';

    for (size_t i = 0; i < n_classes; ++i) {
        // Calculate the length of the bar proportionally to the accuracy
    	int MAX_BAR_LENGTH = 50;
        int barLength = (int)(accuracies[i] * MAX_BAR_LENGTH);
        int limit = MAX_BAR_LENGTH - barLength;

        // Print the label, accuracy, and bar
        printf("Class %-11s [%5.2f%%] : ", labels[i], accuracies[i] * 100);
        for (int j = 0; j < barLength; ++j) {
            putchar(barChar);
        }
        for (int j = 0; j < limit; ++j) {
            putchar(' ');
        }
        printf("|\n");
    }
}
