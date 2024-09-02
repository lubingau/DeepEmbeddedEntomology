#ifndef DATASET_H
#define DATASET_H

#include "ff.h"
#include "xil_printf.h"

#include <stdio.h>
#include <string.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_ORANGE  "\x1b[33m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define DATASET_BUFFER_BASE TENSIL_PLATFORM_DRAM_BUFFER_HIGH

/********************************************************************************************
 *
 * This program defines the classes for the CIFAR10, IMAGENET, BEE306 and TIPU12 datasets.
 * It contains functions to load the different datasets, and parameters for the image.
 *
 ********************************************************************************************/

#define TIPU12

// ///////////////////////////////////////////////////////////////////
#ifdef CIFAR10

#define MODEL_FILE_PATH "8/resnet20v2_cifar_onnx_zyboz7.tmodel"
#define DATASET_FILE_NAME "cifar10.bin"

#define IMAGE_WIDTH 32
#define IMAGE_HEIGHT 32
#define IMAGE_TOTAL_PIXELS (IMAGE_WIDTH * IMAGE_HEIGHT * 3)
#define N_CLASSES 10

#define RED_MEAN 0.491f
#define GREEN_MEAN 0.482f
#define BLUE_MEAN 0.447f

#define RED_STD 1.0f
#define GREEN_STD 1.0f
#define BLUE_STD 1.0f

void load_cifar10(const char *file_name, u8 *buffer, int max_images);

#endif
// ///////////////////////////////////////////////////////////////////
#ifdef IMAGENET

#define MODEL_FILE_PATH "8/resnet50v2_imagenet_onnx_zyboz7.tmodel"
#define DATASET_FILE_NAME "imagenet.bin"

#define IMAGE_WIDTH 224
#define IMAGE_HEIGHT 224
#define IMAGE_TOTAL_PIXELS (IMAGE_WIDTH * IMAGE_HEIGHT * 3)
#define N_CLASSES 1000

#define RED_MEAN 0.485f
#define GREEN_MEAN 0.456f
#define BLUE_MEAN 0.406f

#define RED_STD 0.229f
#define GREEN_STD 0.224f
#define BLUE_STD 0.225f

#endif
// ///////////////////////////////////////////////////////////////////
#ifdef BEE306

#define MODEL_FILE_PATH "8/resnet50_bee306_best_onnx_zyboz7.tmodel"
#define DATASET_FILE_NAME "bee306.bin"

#define IMAGE_WIDTH 224
#define IMAGE_HEIGHT 224
#define IMAGE_TOTAL_PIXELS (IMAGE_WIDTH * IMAGE_HEIGHT * 3)
#define N_CLASSES 306

#define RED_MEAN 0.485f
#define GREEN_MEAN 0.456f
#define BLUE_MEAN 0.406f

#define RED_STD 0.229f
#define GREEN_STD 0.224f
#define BLUE_STD 0.225f

#endif
// ///////////////////////////////////////////////////////////////////
#ifdef TIPU12

#define MODEL_FILE_PATH "8/resnet50_tipu12_onnx_zyboz7.tmodel"
#define DATASET_FILE_NAME "tipu12.bin"

#define IMAGE_WIDTH 224
#define IMAGE_HEIGHT 224
#define IMAGE_TOTAL_PIXELS (IMAGE_WIDTH * IMAGE_HEIGHT * 3)
#define N_CLASSES 12

#define RED_MEAN 0.485f
#define GREEN_MEAN 0.456f
#define BLUE_MEAN 0.406f

#define RED_STD 0.229f
#define GREEN_STD 0.224f
#define BLUE_STD 0.225f

#endif
// ///////////////////////////////////////////////////////////////////

extern const char *classes[];
int load_dataset(const char *file_name, u8 *buffer);

#endif /* DATASET_H */
