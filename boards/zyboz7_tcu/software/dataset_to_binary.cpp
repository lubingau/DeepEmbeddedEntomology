#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <opencv2/opencv.hpp>

using namespace cv;

#define MAX_IMAGES 2000 // Max number of samples per class
#define IMAGE_WIDTH 224
#define IMAGE_HEIGHT 224
#define IMAGE_TOTAL_PIXELS (IMAGE_WIDTH * IMAGE_HEIGHT * 3)

typedef struct {
    uint16_t label;
    uint8_t *rgb;
} ImageData;

typedef struct {
    uint8_t *data;
} Buffer;

void reduce_dataset(const char *binary_file, int max_images) {
    /*
        Reduce the number of images in the binary file to max_images.
        Reduce the weight of the binary file.
    */
    printf("[INFO] Starting to reduce the dataset\n");
    // Open the binary file in read mode
    FILE *bin_file = fopen(binary_file, "rb");
    if (bin_file == NULL) {
        fprintf(stderr, "[ERROR] Failed to open the binary file %s.\n", binary_file);
    }
    // Read the size of the file
    fseek(bin_file, 0, SEEK_END);
    long size = ftell(bin_file);
    fseek(bin_file, 0, SEEK_SET);

    int DATA_SIZE = IMAGE_TOTAL_PIXELS + sizeof(uint16_t);
    int n_images = size / DATA_SIZE; // + sizeof(uint16_t) for the label
    printf("[INFO] Found %d images in the binary file\n", n_images);
    printf("[INFO] Reducing the dataset to %d images\n", max_images);

    // Read the data from the binary file
    uint8_t *ptr = (uint8_t *)malloc(size);
    fread(ptr, sizeof(uint8_t), size, bin_file);
    fclose(bin_file);

    // Load the data
    Buffer datas[n_images];
    for (int i = 0; i < n_images; ++i) {
        datas[i].data = ptr + i * DATA_SIZE;
    }

    // Write the reduced data in the binary file
    bin_file = fopen(binary_file, "wb");
    for (int i = 0; i < max_images; ++i) {
        fwrite(datas[i].data, sizeof(uint8_t), DATA_SIZE, bin_file);
    }

    fclose(bin_file);
    printf("[SUCCESS] Successfully reduced the dataset to %d images\n", max_images);
}


void shuffle_binary_dataset(const char *binary_file) {
    /*
        Shuffle the images in the binary file.
    */
    // Open the binary file in read mode
    FILE *bin_file = fopen(binary_file, "rb");
    if (bin_file == NULL) {
        fprintf(stderr, "[ERROR] Failed to open the binary file %s.\n", binary_file);
    }
    // Read the size of the file
    fseek(bin_file, 0, SEEK_END);
    long size = ftell(bin_file);
    fseek(bin_file, 0, SEEK_SET);

    int DATA_SIZE = IMAGE_TOTAL_PIXELS + sizeof(uint16_t);
    int n_images = size / DATA_SIZE; // + sizeof(uint16_t) for the label
    printf("[INFO] Number of images to shuffle: %d\n", n_images);

    // Read the data from the binary file
    uint8_t *ptr = (uint8_t *)malloc(size);
    fread(ptr, sizeof(uint8_t), size, bin_file);
    fclose(bin_file);

    // Load the data
    Buffer datas[n_images];
    for (int i = 0; i < n_images; ++i) {
        datas[i].data = ptr + i * DATA_SIZE;
    }

    // Shuffle the data
    for (int i = 0; i < n_images; ++i) {
        int j = rand() % n_images;
        Buffer temp = datas[i];
        datas[i] = datas[j];
        datas[j] = temp;
    }

    // Write the shuffled data in the binary file
    bin_file = fopen(binary_file, "wb");
    for (int i = 0; i < n_images; ++i) {
        fwrite(datas[i].data, sizeof(uint8_t), DATA_SIZE, bin_file);
    }
    fclose(bin_file);
    printf("[SUCCESS] Successfully shuffled the binary dataset\n");
}

void images_to_binary(const char *directory, const char *binary_file, const uint16_t label) {
    /*
        Read the images from the class directory and write them to a binary file.
        Preprocess the images by resizing them to 224x224 and converting them to RGB.
        Images are stored in the binary file as follows:
        - 2 bytes for the label (class) of the image (uint16_t)
        - 224x224x3 bytes for the RGB image data in RGB order
    */
    // Open the binary file in append mode
    FILE *bin_file = fopen(binary_file, "ab");
    if (bin_file == NULL) {
        fprintf(stderr, "[ERROR] Failed to open the binary file %s.\n", binary_file);
    }

    // Initialize the images arrays
    ImageData images[MAX_IMAGES];
    int n_images = 0;

    // Open the directory
    DIR *dir = opendir(directory);
    if (dir == NULL) {
        fprintf(stderr, "[ERROR] Failed to open the directory %s.\n", directory);
    }

    // Read the images from the directory
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && n_images < MAX_IMAGES) {
        if (entry->d_type != DT_REG) {
            continue;
        }
        // Save the label (class) of the image
        images[n_images].label = label;

        // Read the image data from the file
        char filename[300];
        sprintf(filename, "%s/%s", directory, entry->d_name);
        Mat img = imread(filename);

        // Preprocess the image
        if (img.channels() == 1) {
            cvtColor(img, img, COLOR_GRAY2BGR);
        }
        cv::resize(img, img, Size(IMAGE_HEIGHT, IMAGE_WIDTH));

        // Put img into uint8_t arrays
        images[n_images].rgb = (uint8_t *)malloc(IMAGE_TOTAL_PIXELS);
        int index = 0;
        for (int i = 0; i < IMAGE_HEIGHT; ++i) {
            for (int j = 0; j < IMAGE_WIDTH; ++j) {
                Vec3b pixel = img.at<Vec3b>(i, j);
                images[n_images].rgb[index++] = pixel[2]; // R
                images[n_images].rgb[index++] = pixel[1]; // G
                images[n_images].rgb[index++] = pixel[0]; // B
            }
        }
        
        n_images++;
    }
    // Close the directory
    closedir(dir);

    // Write the images in the binary file
    for (int i = 0; i < n_images; ++i) {
        fwrite(&(images[i].label), sizeof(uint16_t), 1, bin_file); // Write the label
        fwrite(images[i].rgb, sizeof(uint8_t), IMAGE_TOTAL_PIXELS, bin_file); // Write the image
    }

    // Close the binary file
    fclose(bin_file);

    printf("[INFO] Successfully wrote %d images to %s\n", n_images, binary_file);
}

int compare(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

void dataset_to_binary(const char *dataset_path, const char *binary_file, bool shuffle) {
    /*
        Read the images from the dataset directory and write them to a binary file.
    */
    // Delete the binary file if it exists
    remove(binary_file);

    DIR *dir = opendir(dataset_path);
    if (dir == NULL) {
        fprintf(stderr, "[ERROR] Failed to open the directory %s.\n", dataset_path);
    }

    char *directories_names[1000];
    int n_directories = 0;
    // Get the directories names
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            directories_names[n_directories++] = strdup(entry->d_name);
        }
    }
    printf("[INFO] Number of directories: %d\n", n_directories);
    closedir(dir);
    // Sort the directories names in alphabetical order (mandatory for the labels to be in the correct order)
    qsort(directories_names, n_directories, sizeof(char *), compare);
    printf("[SUCCESS] Directories names sorted\n");

    // Go through the classes directories
    int label = 0;
    for (int i = 0; i < n_directories; ++i) {
        char directory[300];
        sprintf(directory, "%s/%s", dataset_path, directories_names[i]);
        images_to_binary(directory, binary_file, label);
        label++;
    }

    // Shuffle the binary dataset if needed
    if (shuffle) {
        printf("[INFO] Shuffling the binary dataset\n");
        shuffle_binary_dataset(binary_file);
    }
    printf("[SUCCESS] Successfully wrote the dataset to %s\n", binary_file);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <dataset_path> <binary_file> <shuffle>\n", argv[0]);
        // g++ dataset_to_binary.cpp -o dataset_to_binary `pkg-config --cflags --libs opencv4`
        // ./dataset_to_binary Tipu-12/test/ tipu12.bin 1
        return 1;
    }

    char *dataset_path = argv[1];
    char *binary_file = argv[2];
    bool shuffle = atoi(argv[3]);
    int max_images = 100;

    dataset_to_binary(dataset_path, binary_file, shuffle); // Always write the label as uint16_t
    // reduce_dataset(binary_file, max_images); // Comment this line if you don't want to reduce the dataset

    return 0;
}
