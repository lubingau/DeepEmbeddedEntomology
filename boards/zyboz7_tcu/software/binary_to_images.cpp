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

typedef struct {
    uint16_t label;
    uint8_t *red;
    uint8_t *green;
    uint8_t *blue;
} ImageData;

typedef struct {
    uint8_t *data;
} Buffer;

void applyAWB(Mat& img) {
    // Calculate the average of each channel
    Scalar avg = mean(img);

    // Average of the three channels
    double avgGray = (avg[0] + avg[1] + avg[2]) / 3.0;

    // Calculate scale factors
    double scaleB = avgGray / avg[0];
    double scaleG = avgGray / avg[1];
    double scaleR = avgGray / avg[2];

    // Apply scale factors to each channel
    vector<Mat> channels;
    split(img, channels);

    channels[0] *= scaleB;
    channels[1] *= scaleG;
    channels[2] *= scaleR;

    merge(channels, img);
}

void binary_to_images(const char *binary_file, char *directory, int image_width, int image_height) {
    /*
        Read the binary file and save the images WITHOUT labels in the output directory.
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
    // int n_images = size / (TOTAL_PIXELS + sizeof(uint16_t)); // with labels
    int n_images = size / TOTAL_PIXELS;
    printf("Number of images found: %d\n", n_images);

    // Initialize the pointer
    uint8_t *ptr = (uint8_t *)malloc(size);
    fread(ptr, sizeof(uint8_t), size, bin_file);
    fclose(bin_file);
    printf("File readed\n");

    // Read the images from the binary file
    for (int i = 0; i < n_images; ++i) {
        printf("Image %d\n", i);

        Mat img(image_height, image_width, CV_8UC3);
        img.setTo(Scalar(0, 0, 255)); // set the image to red to see if the image is correctly filled (only for debug)
        memcpy(img.data, ptr, TOTAL_PIXELS); // copy the image data to the Mat object
        cvtColor(img, img, COLOR_BGR2RGB); // convert the image to RGB format

        // // Apply auto white balance
        // applyAWB(img);

        // // Apply brightness and contrast
        // int brightness = -35;
        // double contrast = 1.25;
        // img.convertTo(img, -1, contrast, brightness);
        
        imwrite(format("%s/%d.png", directory, i), img); // save the image in the folder

        ptr += TOTAL_PIXELS;
    }

    printf("Images saved in %s directory\n", directory);
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <binary_file> <output_directory> <image_width> <image_height>\n", argv[0]);
        // g++ binary_to_images.cpp -o binary_to_images `pkg-config --cflags --libs opencv4`
        // ./binary_to_images image.bin . 1920 1080
        return 1;
    }

    char *binary_file = argv[1];
    char *output_directory = argv[2];
    int image_width = atoi(argv[3]);
    int image_height = atoi(argv[4]);

    binary_to_images(binary_file, output_directory, image_width, image_height);

    return 0;
}
