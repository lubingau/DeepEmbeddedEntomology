/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright � 2019-2022 Tensil AI Company */

#pragma once

#include <stdbool.h>
#include <xil_types.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_ORANGE  "\x1b[33m"
#define ANSI_COLOR_RESET   "\x1b[0m"

;
void console_set_cursor_position(int x, int y);

void console_clear_screen();

void console_set_foreground_color(int r, int g, int b);

void console_reset_foreground_color();

void console_set_background_color(int r, int g, int b);

void console_reset_background_color();

bool console_get_cursor_position(int *x, int *y);

void image_on_console(const u8 *image, int image_width, int image_height);

void mask_on_console(const u8 *image, int image_width, int image_height);

void printAccuracyBars(const float accuracies[], const char* labels[], int n_classes);
