#include "model_execution.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

/********************************************************************************************
 *
 * This program contains functions to load a model, run it, and evaluate it.
 *
 ********************************************************************************************/

float image_float[IMAGE_TOTAL_PIXELS]; // Memory for the image in float format (problem with the stack)
u16 label_list[N_CLASSES]; // Memory for the labels
u16 predict_list[N_CLASSES]; // Memory for the predictions

const char progress[] = {'-', '\\', '|', '/'}; // Progress bar

void int_to_float(const u8 *buffer, float *output, int TOTAL_PIXELS) //, const float *MEAN) {
{	
	/*
		Convert the image from u8 to float and apply the normalization
	*/
	tensil_error_t error = TENSIL_ERROR_NONE;
	struct stopwatch sw;
	error = stopwatch_start(&sw); // start a timer
	
	for (int i = 0; i < TOTAL_PIXELS; i+=3) {
		output[i] = (CHANNEL_TO_FLOAT(buffer[i]) - RED_MEAN) / RED_STD;
		output[i+1] = (CHANNEL_TO_FLOAT(buffer[i+1]) - GREEN_MEAN) / GREEN_STD;
		output[i+2] = (CHANNEL_TO_FLOAT(buffer[i+2]) - BLUE_MEAN) / BLUE_STD;
	}
	stopwatch_stop(&sw);
	// printf("[SUCCESS] Image converted to float in %.6f seconds\n\r", stopwatch_elapsed_seconds(&sw));
}

size_t argmax(size_t size, const float *buffer)
{	
	/*
		Return the index of the maximum value in the buffer
	*/
    if (!size) {
        return -1;
	}
    float max = buffer[0];
    size_t max_i = 0;

    for (size_t i = 1; i < size; i++) {
        if (buffer[i] > max) {
            max = buffer[i];
            max_i = i;
        }
	}

    return max_i;
}


void calculate_f1_score(u16 *labels, u16 *predictions, int n_samples) {
	/*
		Calculate the F1 score for each class and the macro F1 score
	*/
    int TP[N_CLASSES] = {0}, FP[N_CLASSES] = {0}, FN[N_CLASSES] = {0};
    float precision[N_CLASSES] = {0}, recall[N_CLASSES] = {0}, f1_score[N_CLASSES] = {0};

    // Count true positives, false positives and false negatives for each class
    for (int i = 0; i < n_samples; i++) {
		if (predictions[i] == labels[i]) {
			TP[i]++;
		} else {
			FP[i]++;
			FN[i]++;
		}
    }

    // Calculate precision, recall and F1 score for each class
    for (int i = 0; i < N_CLASSES; i++) {
		if (TP[i] + FP[i] > 0) {
			precision[i] = (float)TP[i] / (TP[i] + FP[i]);
		} else {
			precision[i] = 0.0f;
		}

		if (TP[i] + FN[i] > 0) {
			recall[i] = (float)TP[i] / (TP[i] + FN[i]);
		} else {
			recall[i] = 0.0f;
		}

		if (precision[i] + recall[i] > 0) {
			f1_score[i] = 2.0f * (precision[i] * recall[i]) / (precision[i] + recall[i]);
		} else {
			f1_score[i] = 0.0f;
		}
        printf("[INFO] Class %d: Precision = %.2f, Recall = %.2f, F1 Score = %.2f\n", i, precision[i], recall[i], f1_score[i]);
    }

    // Calculate the macro F1 score
    float macro_f1_score = 0.0f;
	for (int i = 0; i < N_CLASSES; i++) {
		macro_f1_score += f1_score[i];
	}
	macro_f1_score /= N_CLASSES;
    printf("[INFO] Macro F1 Score = %.2f\n\r", 100.0*macro_f1_score);
}

int save_predictions(const char *file_name, u16 *labels, u16 *predictions, int n_images)
{	
	/*
		Save the predictions to a file on the SD card
	*/
	bool error = false;
    // SD card setup
	FRESULT res;
	FIL fil;
	FILINFO fno;
	UINT bytes_read;

    // Open the file
	res = f_open(&fil, file_name, FA_CREATE_ALWAYS | FA_WRITE);
	if (res != FR_OK) {
		xil_printf(ANSI_COLOR_RED "[ERROR] Error opening file: %d\n\r" ANSI_COLOR_RESET, res);
		error = true;
		goto ERROR;
	}

    // Write the datas
	f_write(&fil, labels, n_images * sizeof(u16), &bytes_read);
	f_write(&fil, predictions, n_images * sizeof(u16), &bytes_read);

	// Close the file
	f_close(&fil);

    printf("[SUCCESS] Predictions saved to %s\n", file_name);

    return 0;

ERROR:
	if (error) {xil_printf(ANSI_COLOR_RED "[ERROR] Failed to save the predictions\n\r" ANSI_COLOR_RESET);}
}

tensil_error_t load_model(const char *model_path, struct tensil_driver *driver, struct tensil_model *model)
{	
	/*
		Load the model from a file and load it in the TCU
	*/
	tensil_error_t error = TENSIL_ERROR_NONE;
	struct stopwatch sw;
	error = stopwatch_start(&sw); // start a timer

	error = tensil_driver_init(driver);
	if (error) {
		xil_printf(ANSI_COLOR_RED "[ERROR] Error initializing the driver\n\r" ANSI_COLOR_RESET);
		return error;}
	xil_printf("[INFO] Driver initialized\n\r");

	error = tensil_model_from_file(model, model_path);
	if (error) {
		xil_printf(ANSI_COLOR_RED "[ERROR] Error loading the model from file\n\r" ANSI_COLOR_RESET);
		return error;}
	xil_printf("[INFO] Model loaded from file\n\r");

	error = tensil_driver_load_model(driver, model);
	if (error) {
		xil_printf(ANSI_COLOR_RED "[ERROR] Error loading the model in the TCU\n\r" ANSI_COLOR_RESET);
		return error;}
	
	stopwatch_stop(&sw);
	float seconds = stopwatch_elapsed_seconds(&sw);
	printf("[SUCCESS] Model loaded in the TCU in %.2f seconds\n\r", seconds);
	return error;
}

tensil_error_t run_model(struct tensil_driver *driver, const struct tensil_model *model, const char *input_node, const char *output_node, const float *image, int TOTAL_PIXELS, int n_classes, float *result)
{	
	/*
		Run the model on a single image
	*/
	tensil_error_t error = TENSIL_ERROR_NONE;
	// struct stopwatch sw;
	// error = stopwatch_start(&sw); // start a timer

	// Load the image in the model
	for (int i = 0; i < TOTAL_PIXELS; i+=3) {
		float pixel[] = {image[i], image[i+1], image[i+2]};
		error = tensil_driver_load_model_input_vector_scalars(driver, model, input_node, i/3, 3, pixel); // load the image in the model
	}
	// stopwatch_stop(&sw);
	// printf("[INFO] IMAGE loaded in %.6f seconds\n\r", stopwatch_elapsed_seconds(&sw));
	// error = stopwatch_start(&sw);

	if (error) {
		xil_printf(ANSI_COLOR_RED "[ERROR] Error loading the model input\n\r" ANSI_COLOR_RESET);
		return error;}
	// Run the model
	error = tensil_driver_run(driver, NULL); // run the model
	if (error) {
		xil_printf(ANSI_COLOR_RED "[ERROR] Error running the model\n\r" ANSI_COLOR_RESET);
		return error;}
	// Get the results
	error = tensil_driver_get_model_output_scalars(driver, model, output_node, n_classes, result); // extract the results and put it in an array. Identity is the name of the output node
	if (error)  {
		xil_printf(ANSI_COLOR_RED "[ERROR] Error getting the model output\n\r" ANSI_COLOR_RESET);
		return error;}
	// stopwatch_stop(&sw);
	// printf("[SUCCESS] Model executed in %.6f seconds\n\r", stopwatch_elapsed_seconds(&sw));
	// Print the results
	// xil_printf("\n\rResult:\n\r");
	// error = tensil_driver_print_model_output_vectors(driver, model, output_node);
	// if (error)  {return error;}

	return error;
}

tensil_error_t evaluate_model(struct tensil_driver *driver, const struct tensil_model *model, const char *input_node, const char *output_node, const u8 *dataset, int n_images, int TOTAL_PIXELS, int n_classes)
{	
	/*
		Evaluate the model on a dataset
	*/
	xil_printf("[INFO] Evaluating the model\n\r");
	tensil_error_t error = TENSIL_ERROR_NONE;

	float result[n_classes];
	int n_correct = 0;
	int count_classes[n_classes] = {0};
	float accuracies[n_classes] = {0};
	int n_correct_class[n_classes] = {0};
	int image_width = sqrt(TOTAL_PIXELS / 3);
	float tcu_time = 0.0;
	float total_time = 0.0;

	u8 *ptr = (u8 *)dataset;

	console_clear_screen();

	for (int i = 0; i < n_images; i++) {
		struct stopwatch sw0;
		error = stopwatch_start(&sw0);
		// Get the label
		u8 label0 = *ptr;
		ptr++;
		u8 label1 = *ptr;
		ptr++;
		u16 label = (label1 << 8) | label0;
		count_classes[label]++;
		// Convert the image to float
		int_to_float(ptr, image_float, TOTAL_PIXELS);

		struct stopwatch sw;
        error = stopwatch_start(&sw);

		// Run the model
		error = run_model(driver, model, input_node, output_node, image_float, TOTAL_PIXELS, n_classes, result);
		if (error) {return error;}

		stopwatch_stop(&sw);
        tcu_time += stopwatch_elapsed_seconds(&sw);
        stopwatch_stop(&sw0);
		total_time += stopwatch_elapsed_seconds(&sw0);

		size_t predict_label = argmax(n_classes, result);
		if ((u16)predict_label == label) {
			n_correct++;
			n_correct_class[label]++;
		}

		label_list[i] = label;
		predict_list[i] = predict_label;

		console_set_cursor_position(1, 1);
		printf("%d/%d: %.3f fps (%.5f inf %.5f tot) %c\n", i+1, n_images, (float)(i+1) / total_time, tcu_time/(i+1), total_time/(i+1), progress[i % 4]);

		if (i % 2 == 0) { // Print the image every 10 images
			printf("\nImage:");
			image_on_console(ptr, image_width, image_width);
			// printf("True class = %s, Predicted class = %s                                      \n", classes[label], classes[predict_label]);
			printf("True label = %d, Predicted label = %d             \n", label, predict_label);
			printf("True class = %s, Predicted class = %s             \n", classes[label], classes[predict_label]);
			printf("Accuracy: %.2f\n\r", 100.0f*(float)n_correct / (i+1));
			console_reset_foreground_color();
		}
		ptr += TOTAL_PIXELS;
	}
	// display 32* \n to clear the screen
	for (int i = 0; i < 40; i++) {
		xil_printf("\n");
	}
	printf("[SUCCESS] Model evaluated\n\r");
	printf("[INFO] Total execution time %.2f seconds\n\r", total_time);
	printf("[INFO] TCU execution time %.2f seconds\n\r", tcu_time);
	printf("[INFO] Total FPS %.3f\n\r", n_images/total_time);
	printf("[INFO] TCU FPS %.3f\n\r", n_images/tcu_time);
	for (int i = 0; i < n_classes; i++) {
		accuracies[i] = n_correct_class[i]/count_classes[i];
	}
	printAccuracyBars(accuracies, classes, n_classes);
	float accuracy = 100.0f*n_correct / n_images;
	printf("[INFO] Global accuracy: %.2f\n\r", accuracy);
	calculate_f1_score(label_list, predict_list, n_images);
	save_predictions("predictions.bin", label_list, predict_list, n_images);

	return error;
}
