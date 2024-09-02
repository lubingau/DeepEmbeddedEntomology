#ifndef MODEL_EXECUTION_H
#define MODEL_EXECUTION_H

#include "../tensil/dram.h"
#include "../tensil/driver.h"
#include "../tensil/instruction.h"
#include "../tensil/model.h"
#include "../tensil/tcu.h"

#include "../console/console.h"
#include "../console/stopwatch.h"

#include "model_execution.h"
#include "dataset.h"
#include "dataset.h"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_ORANGE  "\x1b[33m"
#define ANSI_COLOR_RESET   "\x1b[0m"

/********************************************************************************************
 *
 * This program contains functions to load a model, run it, and evaluate it.
 *
 ********************************************************************************************/

#define CHANNEL_TO_FLOAT(v) ((float)v / 255.0)

void int_to_float(const u8 *buffer, float *output, int TOTAL_PIXELS);//, const float *MEAN);
size_t argmax(size_t size, const float *buffer);
void calculate_f1_score(u16 *labels, u16 *predictions, int n_samples);
int save_predictions(const char *file_name, u16 *labels, u16 *predictions, int n_samples);

tensil_error_t load_model(const char *model_path, struct tensil_driver *driver, struct tensil_model *model);
tensil_error_t run_model(struct tensil_driver *driver, const struct tensil_model *model, const char *input_node, const char *output_node, const float *image, int TOTAL_PIXELS, int n_classes, float *result);
tensil_error_t evaluate_model(struct tensil_driver *driver, const struct tensil_model *model, const char *input_node, const char *output_node, const u8 *dataset, int n_images, int TOTAL_PIXELS, int n_classes);

#endif /* MODEL_EXECUTION_H */
