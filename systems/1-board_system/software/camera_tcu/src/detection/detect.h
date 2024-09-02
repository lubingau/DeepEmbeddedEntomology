#ifndef DETECT_H
#define DETECT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../console/console.h"
#include "../console/stopwatch.h"
#include "../model/dataset.h"
#include "image_processing.h"
#include "camera.h"

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

#define DETECTION_INTERVAL_SECOND 10
#define NOISE_THRESHOLD 100
#define MAX_CLUSTERS 5
#define MAX_LOCATIONS 2000

/********************************************************************************************
 *
 * This program implements the detection algorithm for the insect detection system.
 *
 ********************************************************************************************/

// Function declarations
void background_subtraction(const u8 *image, const u8 *background_image, u8 *new_image, int image_width, int image_height);
void mask_image(const u8 *new_image, u8 *bin_image, int image_width, int image_height);
void binarize_image(const u8 *new_image, u8 *bin_image, int image_width, int image_height);
void detectAndFilterLocations(const u8 *bin_image, int image_width, int image_height, int num, int countmax, u16 *locations, int *num_locations);
void massCenter(const u16 *locations2, int numPoints, u16 *Xcenter, u16 *Ycenter);
void MinMaxLocations(const u16 *locations2, int numPoints, u16 *Xmin, u16 *Ymin, u16 *Xmax, u16 *Ymax);
void massCenterBidimensional(const int cluster[][2], int numPoints, u16 *Xcenter, u16 *Ycenter);
void MinMaxLocationsBidimensional(const int cluster[][2], int numPoints, u16 *Xmin, u16 *Ymin, u16 *Xmax, u16 *Ymax);
//void detectClusters(const u16 *locations, int num_locations, int var, int area, int *idx);
//void detectClusters(const u16 *locations, int num_locations, int var, int area, int *idx, int clusters[MAX_CLUSTERS][MAX_LOCATIONS][2], int *cluster_sizes);
void detectClusters(const u16 *locations, int num_locations, int dist, int clusVar, int *idx,
                    int unique_mass_centers[MAX_CLUSTERS][2], int grouped_clusters[MAX_CLUSTERS][MAX_LOCATIONS][2],
                    int min_max_coordinates[MAX_CLUSTERS][4]);
void binarize_image_only(const u8 *new_image, u8 *bin_image, int threshold, int image_width, int image_height);
void runDetection (const u8 *image, const u8 *background_image,const u8 *imageRGB, u8 *crop,
		int unique_mass_centers[MAX_CLUSTERS][2],  int min_max_coordinates[MAX_CLUSTERS][4],
		int grouped_clusters[MAX_CLUSTERS][MAX_LOCATIONS][2],int dimensions[MAX_CLUSTERS][2],
		u16 *locations, int *num_locations, u8 sizing_factor);
void printCenterMinMaxLocations (u16 *locations, int num_locations);
void printClusterCrops(const u8 *imageRGB, u8 *crop, int unique_mass_centers[MAX_CLUSTERS][2],
		int min_max_coordinates[MAX_CLUSTERS][4], u8 *resized_image,
		int dimensions[MAX_CLUSTERS][2],int console_height, int console_width, u8 sizing_factor);
void detectionDebug (const u8 *image, const u8 *background_image,const u8 *imageRGB, u8 *crop,
		int unique_mass_centers[MAX_CLUSTERS][2],  int min_max_coordinates[MAX_CLUSTERS][4],
		int grouped_clusters[MAX_CLUSTERS][MAX_LOCATIONS][2],int dimensions[MAX_CLUSTERS][2],
		u16 *locations, int *num_locations, u8 sizing_factor);
void MassCenterCrop(const u8 *imageRGB, u8 *crop, int unique_mass_centers[MAX_CLUSTERS][2],
		int min_max_coordinates[MAX_CLUSTERS][4], u8 *resized_image,
		int dimensions[MAX_CLUSTERS][2],int console_height, int console_width, u8 sizing_factor);
void MinMaxCrop(const u8 *imageRGB, u8 *crop, int unique_mass_centers[MAX_CLUSTERS][2],
		int min_max_coordinates[MAX_CLUSTERS][4], u8 *resized_image,
		int dimensions[MAX_CLUSTERS][2],int console_height, int console_width, u8 sizing_factor);

#endif // DETECT_H
