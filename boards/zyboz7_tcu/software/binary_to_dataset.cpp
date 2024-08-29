#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>

#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;


void binary_to_dataset(const char *binary_file, char *directory, int image_width, int image_height, int max_images) {
    /*
        Read the binary file and save the images and labels in the output directory.
    */
    // Open the binary file in read mode
    FILE *bin_file = fopen(binary_file, "rb");
    if (bin_file == NULL) {
        fprintf(stderr, "Failed to open the binary file %s.\n", binary_file);
    }

    // Remove the "/" at the end of the directory if exists
    int len = strlen(directory);
    if (directory[len - 1] == '/') {
        directory[len - 1] = '\0';
    }

    // Create the directory if not exists
    struct stat st = {0};
    if (stat(directory, &st) == -1) {
        mkdir(directory, 0700);
    }

    // Read the size of the file
    fseek(bin_file, 0, SEEK_END);
    long size = ftell(bin_file);
    fseek(bin_file, 0, SEEK_SET);
    printf("Size of the file: %ld\n", size);

    // Calculate the number of images
    int TOTAL_PIXELS = image_width * image_height * 3;
    int n_images = size / (TOTAL_PIXELS + sizeof(uint16_t));
    printf("Number of images found: %d\n", n_images);
    n_images = n_images > max_images ? max_images : n_images;
    printf("Number of images to read: %d\n", n_images);

    // Initialize the pointer
    uint8_t *ptr = (uint8_t *)malloc(size);
    fread(ptr, sizeof(uint8_t), size, bin_file);
    fclose(bin_file);
    printf("File readed\n");

    // Read the images from the binary file
    for (int i = 0; i < n_images; ++i) {
        printf("Image %d\n", i);

        uint8_t label0 = *ptr; // extract the label (the class)
        ptr += 1;
        uint8_t label1 = *ptr;
        ptr += 1;
        uint16_t label = (label1 << 8) | label0;
        printf("------- label %d\n", label);

        Mat img(image_height, image_width, CV_8UC3);
        // img.setTo(Scalar(0, 0, 255)); // set the image to red to see if the image is correctly filled (only for debug)
        memcpy(img.data, ptr, TOTAL_PIXELS); // copy the image data to the Mat object
        cvtColor(img, img, COLOR_BGR2RGB); // convert the image to RGB format

        // Create the "label" directory if not exists
        char label_directory[100];
        sprintf(label_directory, "%s/%d", directory, label);
        struct stat st = {0};
        if (stat(label_directory, &st) == -1) {
            mkdir(label_directory, 0700);
        }        

        // Save the image in the directory
        imwrite(format("%s/%d/%d.png", directory, label, i), img); // save the image in the folder

        ptr += TOTAL_PIXELS;
    }

    printf("Successfully read %d images from %s\n", n_images, binary_file);
    printf("Images saved in %s directory\n", directory);
}


int main(int argc, char *argv[]) {
    if (argc != 6) {
        fprintf(stderr, "Usage: %s <binary_file> <output_directory> <image_width> <image_height> <max_images>\n", argv[0]);
        // g++ binary_to_dataset.cpp -o binary_to_dataset `pkg-config --cflags --libs opencv4`
        // ./binary_to_dataset tipu.bin test/ 224 224 5
        return 1;
    }

    char *binary_file = argv[1];
    char *output_directory = argv[2];
    int image_width = atoi(argv[3]);
    int image_height = atoi(argv[4]);
    int max_images = atoi(argv[5]);

    binary_to_dataset(binary_file, output_directory, image_width, image_height, max_images);

    return 0;
}
