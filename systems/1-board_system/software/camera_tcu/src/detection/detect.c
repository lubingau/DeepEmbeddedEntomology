#include "detect.h"
#include "xil_printf.h"

/********************************************************************************************
 *
 * This program implements the detection algorithm for the insect detection system.
 *
 ********************************************************************************************/


void background_subtraction(const u8 *image, const u8 *background_image, u8 *new_image, int image_width, int image_height) {
	/*
		Subtract the background image from the current image
	*/
    int size = image_width * image_height;
    for (int i = 0; i < size; i++) {
        new_image[i] = (u8)(image[i] - background_image[i]) + 100;
        if (new_image[i] < 0){
            new_image[i]=0;
        }
    }
}

void mask_image(const u8 *new_image, u8 *bin_image, int image_width, int image_height) {
	/*
		Binarize the image
	*/
    int size = image_width * image_height;
    int threshold = 50;
    for (int i = 0; i < size; i++) {
        if (new_image[i] > threshold) {
            bin_image[i] =  255;
        } else {
            bin_image[i] =  0;
        }
    }
}

void binarize_image(const u8 *new_image, u8 *bin_image, int image_width, int image_height) {
	/*
		Binarize the image
	*/
    int size = image_width * image_height;
    for (int i = 0; i < size; i++) {
        bin_image[i] = (new_image[i] == 0);
    }
}

void binarize_image_only(const u8 *new_image, u8 *bin_image, int threshold, int image_width, int image_height) {
	/*
		Binarize the image
	*/
	int size = image_width * image_height;
	    for (int i = 0; i < size; i++) {
	        if (new_image[i] > threshold) {
	            bin_image[i] =  0;
	        } else {
	            bin_image[i] =  1;
	        }
	    }
}


void detectAndFilterLocations(const u8 *bin_image, int image_width, int image_height, int num, int countmax, u16 *locations, int *num_locations) {
	/*
		Detect locations in the binarized image
	*/
    *num_locations = 0;

    for (int i = 0; i < image_height; i++) {
        for (int j = 0; j < image_width; j++) {
            if (bin_image[i * image_width + j] == 1 && j + num <= image_width && i + num <= image_height) {
                int count_h = 0;
                for (int k = 1; k <= num; k++) {
                    count_h += bin_image[i * image_width + j + k];
                }

                int count_v = 0;
                for (int k = 1; k <= num; k++) {
                    count_v += bin_image[(i + k) * image_width + j];
                }

                int count = count_h + count_v;
                if (count > countmax) {
                    locations[(*num_locations) * 2] = (u16)i;
                    locations[(*num_locations) * 2 + 1] = (u16)j;
                    (*num_locations)++;
                }
            }
        }
    }
}

void massCenter(const u16 *locations2, int numPoints, u16 *Xcenter, u16 *Ycenter) {
	/*
		Calculate the mass center of the detected locations
	*/
    u32 sumX = 0;
    u32 sumY = 0;

    for (int i = 0; i < numPoints; i++) {
        sumX += locations2[2 * i];     // Sum of X coordinates
        sumY += locations2[2 * i + 1]; // Sum of Y coordinates
    }

    *Xcenter = (u16)(sumX / numPoints);
    *Ycenter = (u16)(sumY / numPoints);
}

void MinMaxLocations(const u16 *locations2, int numPoints, u16 *Xmin, u16 *Ymin, u16 *Xmax, u16 *Ymax) {
	/*
		Calculate the minimum and maximum coordinates of the detected locations
	*/
    // Initialize the min and max values to appropriate starting values
    *Xmin = 1920;
    *Ymin = 1080;
    *Xmax = 0;
    *Ymax = 0;

    for (int i = 0; i < numPoints; i++) {
        u16 x = locations2[2 * i];
        u16 y = locations2[2 * i + 1];

        if (x < *Xmin) *Xmin = x;
        if (y < *Ymin) *Ymin = y;
        if (x > *Xmax) *Xmax = x;
        if (y > *Ymax) *Ymax = y;
    }
}

void massCenterBidimensional(const int cluster[][2], int numPoints, u16 *Xcenter, u16 *Ycenter) {
	/*
		Calculate the mass center of the detected locations
	*/
    u32 sumX = 0;
    u32 sumY = 0;

    for (int i = 0; i < numPoints; i++) {
        sumX += cluster[i][0];     // Sum of X coordinates
        sumY += cluster[i][1];     // Sum of Y coordinates
    }

    *Xcenter = (u16)(sumX / numPoints);
    *Ycenter = (u16)(sumY / numPoints);
}

void MinMaxLocationsBidimensional(const int cluster[][2], int numPoints, u16 *Xmin, u16 *Ymin, u16 *Xmax, u16 *Ymax) {
	/*
		Calculate the minimum and maximum coordinates of the detected locations
	*/
    // Initialize the min and max values to appropriate starting values
    *Xmin = 1920;
    *Ymin = 1080;
    *Xmax = 0;
    *Ymax = 0;

    for (int i = 0; i < numPoints; i++) {
        u16 x = cluster[i][0];
        u16 y = cluster[i][1];

        if (x < *Xmin) *Xmin = x;
        if (y < *Ymin) *Ymin = y;
        if (x > *Xmax) *Xmax = x;
        if (y > *Ymax) *Ymax = y;
    }
}

void detectClusters(const u16 *locations, int num_locations, int dist, int clusVar, int *idx,
                    int unique_mass_centers[MAX_CLUSTERS][2], int grouped_clusters[MAX_CLUSTERS][MAX_LOCATIONS][2],
                    int min_max_coordinates[MAX_CLUSTERS][4]) {
	/*
		Detect clusters in the detected locations
	*/
    int C = 1;
    memset(idx, 0, num_locations * sizeof(int));

    // Assign initial unique labels to each point
    for (int i = 0; i < num_locations; i++) {
        idx[i] = C;
        C++;
    }

    // Clustering based on distance
    for (int i = 0; i < num_locations; i++) {
        for (int j = 0; j < num_locations; j++) {
            if (i != j) { // Skip comparing a point with itself
                int Xdiff = abs(locations[j * 2] - locations[i * 2]);
                int Ydiff = abs(locations[j * 2 + 1] - locations[i * 2 + 1]);
                if (Xdiff < dist && Ydiff < dist) {
                    idx[j] = idx[i]; // Assign the same cluster label
                }
            }
        }
    }

    // Calculate mass centers for each cluster
    int num_unique_clusters = 0;
    int cluster_map[MAX_CLUSTERS] = {0}; // Map old cluster labels to new ones

    for (int i = 0; i < num_locations; i++) {
        if (cluster_map[idx[i]] == 0) {
            cluster_map[idx[i]] = num_unique_clusters + 1;
            num_unique_clusters++;
        }
        idx[i] = cluster_map[idx[i]] - 1;
    }

    int mass_centers[MAX_CLUSTERS][2] = {0};
    int cluster_counts[MAX_CLUSTERS] = {0};

    for (int i = 0; i < num_locations; i++) {
        int cluster_id = idx[i];
        mass_centers[cluster_id][0] += locations[i * 2];
        mass_centers[cluster_id][1] += locations[i * 2 + 1];
        cluster_counts[cluster_id]++;
    }

    for (int i = 0; i < num_unique_clusters; i++) {
        if (cluster_counts[i] > 0) {
            mass_centers[i][0] /= cluster_counts[i];
            mass_centers[i][1] /= cluster_counts[i];
        }
    }

    // Group clusters with close mass centers
    int used_indices[MAX_CLUSTERS] = {0};
    int num_grouped_clusters = 0;

    for (int i = 0; i < num_unique_clusters; i++) {
        if (used_indices[i]) continue;

        int close_indices[MAX_CLUSTERS] = {0};
        int num_close_indices = 0;
        close_indices[num_close_indices++] = i;

        for (int j = i + 1; j < num_unique_clusters; j++) {
            int a = abs(mass_centers[j][0] - mass_centers[i][0]);
            int b = abs(mass_centers[j][1] - mass_centers[i][1]);
            if (a < clusVar && b < clusVar) {
                close_indices[num_close_indices++] = j;
                used_indices[j] = 1;
            }
        }

        int grouped_mass_center[2] = {0};
        for (int j = 0; j < num_close_indices; j++) {
            int index = close_indices[j];
            grouped_mass_center[0] += mass_centers[index][0];
            grouped_mass_center[1] += mass_centers[index][1];
        }
        grouped_mass_center[0] /= num_close_indices;
        grouped_mass_center[1] /= num_close_indices;

        unique_mass_centers[num_grouped_clusters][0] = grouped_mass_center[0];
        unique_mass_centers[num_grouped_clusters][1] = grouped_mass_center[1];

        int cluster_points[MAX_LOCATIONS][2] = {0};
        int total_points = 0;
        for (int j = 0; j < num_close_indices; j++) {
            int index = close_indices[j];
            for (int k = 0; k < num_locations; k++) {
                if (idx[k] == index) {
                    cluster_points[total_points][0] = locations[k * 2];
                    cluster_points[total_points][1] = locations[k * 2 + 1];
                    total_points++;
                }
            }
        }

        for (int j = 0; j < total_points; j++) {
            grouped_clusters[num_grouped_clusters][j][0] = cluster_points[j][0];
            grouped_clusters[num_grouped_clusters][j][1] = cluster_points[j][1];
        }

        int Xmin = cluster_points[0][0];
        int Xmax = cluster_points[0][0];
        int Ymin = cluster_points[0][1];
        int Ymax = cluster_points[0][1];

        for (int j = 1; j < total_points; j++) {
            if (cluster_points[j][0] < Xmin) Xmin = cluster_points[j][0];
            if (cluster_points[j][0] > Xmax) Xmax = cluster_points[j][0];
            if (cluster_points[j][1] < Ymin) Ymin = cluster_points[j][1];
            if (cluster_points[j][1] > Ymax) Ymax = cluster_points[j][1];
        }

        min_max_coordinates[num_grouped_clusters][0] = Xmin;
        min_max_coordinates[num_grouped_clusters][1] = Xmax;
        min_max_coordinates[num_grouped_clusters][2] = Ymin;
        min_max_coordinates[num_grouped_clusters][3] = Ymax;

        num_grouped_clusters++;
    }

    // Print results
//	   for (int i = 0; i < num_grouped_clusters; i++) {
//		   printf("Grouped Cluster %d: Center of Mass at (X: %d, Y: %d)\n", i, unique_mass_centers[i][0], unique_mass_centers[i][1]);
//		   printf("Grouped Cluster %d: Min (X: %d, Y: %d), Max (X: %d, Y: %d)\n", i, min_max_coordinates[i][0], min_max_coordinates[i][2], min_max_coordinates[i][1], min_max_coordinates[i][3]);
//	   }

	   // Re-group clusters with close mass centers using only unique_mass_centers and min_max_coordinates
	      int final_mass_centers[MAX_CLUSTERS][2] = {0};
	      int final_min_max_coordinates[MAX_CLUSTERS][4] = {0};
	      int final_used_indices[MAX_CLUSTERS] = {0};
	      int num_final_grouped_clusters = 0;
	      int clusVar2=75;

	      for (int i = 0; i < num_grouped_clusters; i++) {
	          if (final_used_indices[i]) continue;

	          int final_close_indices[MAX_CLUSTERS] = {0};
	          int num_final_close_indices = 0;
	          final_close_indices[num_final_close_indices++] = i;

	          for (int j = i + 1; j < num_grouped_clusters; j++) {
	              int a = abs(unique_mass_centers[j][0] - unique_mass_centers[i][0]);
	              int b = abs(unique_mass_centers[j][1] - unique_mass_centers[i][1]);
	              if (a+b<clusVar2) {
	                  final_close_indices[num_final_close_indices++] = j;
	                  final_used_indices[j] = 1;
	              }
	          }

	          int final_grouped_mass_center[2] = {0};
	          int final_min_x = min_max_coordinates[final_close_indices[0]][0];
	          int final_max_x = min_max_coordinates[final_close_indices[0]][1];
	          int final_min_y = min_max_coordinates[final_close_indices[0]][2];
	          int final_max_y = min_max_coordinates[final_close_indices[0]][3];

	          for (int j = 0; j < num_final_close_indices; j++) {
	              int index = final_close_indices[j];
	              final_grouped_mass_center[0] += unique_mass_centers[index][0];
	              final_grouped_mass_center[1] += unique_mass_centers[index][1];
	              if (min_max_coordinates[index][0] < final_min_x) final_min_x = min_max_coordinates[index][0];
	              if (min_max_coordinates[index][1] > final_max_x) final_max_x = min_max_coordinates[index][1];
	              if (min_max_coordinates[index][2] < final_min_y) final_min_y = min_max_coordinates[index][2];
	              if (min_max_coordinates[index][3] > final_max_y) final_max_y = min_max_coordinates[index][3];
	          }

	          final_grouped_mass_center[0] /= num_final_close_indices;
	          final_grouped_mass_center[1] /= num_final_close_indices;

	          final_mass_centers[num_final_grouped_clusters][0] = final_grouped_mass_center[0];
	          final_mass_centers[num_final_grouped_clusters][1] = final_grouped_mass_center[1];

	          final_min_max_coordinates[num_final_grouped_clusters][0] = final_min_x;
	          final_min_max_coordinates[num_final_grouped_clusters][1] = final_max_x;
	          final_min_max_coordinates[num_final_grouped_clusters][2] = final_min_y;
	          final_min_max_coordinates[num_final_grouped_clusters][3] = final_max_y;

	          num_final_grouped_clusters++;
	      }


	      // Set unused clusters to zero
	          for (int i = num_final_grouped_clusters; i < MAX_CLUSTERS; i++) {
	              memset(unique_mass_centers[i], 0, sizeof(unique_mass_centers[i]));
	              memset(min_max_coordinates[i], 0, sizeof(min_max_coordinates[i]));
	          }

	      // Copy final results back to original arrays
	      for (int i = 0; i < num_final_grouped_clusters; i++) {
	          unique_mass_centers[i][0] = final_mass_centers[i][0];
	          unique_mass_centers[i][1] = final_mass_centers[i][1];
	          min_max_coordinates[i][0] = final_min_max_coordinates[i][0];
	          min_max_coordinates[i][1] = final_min_max_coordinates[i][1];
	          min_max_coordinates[i][2] = final_min_max_coordinates[i][2];
	          min_max_coordinates[i][3] = final_min_max_coordinates[i][3];
	      }

	      // Print results
	      for (int i = 0; i < num_final_grouped_clusters; i++) {
	          printf("Grouped Cluster %d: Center of Mass at (X: %d, Y: %d)\n", i+1, unique_mass_centers[i][0], unique_mass_centers[i][1]);
	          printf("Grouped Cluster %d: Min (X: %d, Y: %d), Max (X: %d, Y: %d)\n", i+1, min_max_coordinates[i][0], min_max_coordinates[i][2], min_max_coordinates[i][1], min_max_coordinates[i][3]);
	      }
}

void runDetection (const u8 *image, const u8 *background_image,const u8 *imageRGB, u8 *crop,
		int unique_mass_centers[MAX_CLUSTERS][2],  int min_max_coordinates[MAX_CLUSTERS][4],
		int grouped_clusters[MAX_CLUSTERS][MAX_LOCATIONS][2],int dimensions[MAX_CLUSTERS][2],
		u16 *locations, int *num_locations, u8 sizing_factor){
		/*
			Run the insect detection algorithm
		*/

		u8 new_image[FRAME_TOTAL_PIXELS/3];
		u8 resized_image[FRAME_TOTAL_PIXELS];

		//Detection Parameters
		int num = 10; // Variable for the detection function
		int countmax = 5; // Threshold for the detection function
		int img_width_resized=FRAME_WIDTH/sizing_factor;
		int img_height_resized=FRAME_HEIGHT/sizing_factor;
		int dist=8;
		int clusVar=50;
		int threshold=50;

		tensil_error_t error = TENSIL_ERROR_NONE;
		struct stopwatch sw;
		error = stopwatch_start(&sw); // start a timer

		background_subtraction(image, background_image, new_image, FRAME_WIDTH, FRAME_HEIGHT);
		binarize_image_only(new_image, new_image, threshold, FRAME_WIDTH, FRAME_HEIGHT);
		resize_mask(new_image, resized_image, FRAME_WIDTH, FRAME_HEIGHT, img_width_resized, img_height_resized);
		detectAndFilterLocations(resized_image, img_width_resized, img_height_resized, num, countmax, locations, num_locations);
		printf("Detected locations (total: %d):\n", *num_locations);
		if (*num_locations==0){
			printf("[INFO] No locations detected\n");
		}
		else if (*num_locations>MAX_LOCATIONS){
			printf("[INFO] Too many locations detected\n");
			printf("[INFO] Number of locations will be limited to %d\n", MAX_LOCATIONS);
			*num_locations=MAX_LOCATIONS;
		}
		int idx[*num_locations];
		detectClusters(locations, *num_locations, dist, clusVar, idx, unique_mass_centers, grouped_clusters, min_max_coordinates);

		stopwatch_stop(&sw);
//		printf("[INFO] Insect Detection in %.6f seconds\n\r", stopwatch_elapsed_seconds(&sw));
		error = stopwatch_start(&sw);


}

void printCenterMinMaxLocations (u16 *locations, int num_locations) {
	/*
		Print the center and min/max locations of the detected locations
	*/
	u16 Xmin, Xmax, Ymin, Ymax, Xcenter, Ycenter;

	massCenter(locations, num_locations, &Ycenter, &Xcenter);
	printf("Center Location X: %d // Location Y: %d):\n", Xcenter, Ycenter);
	MinMaxLocations(locations, num_locations, &Ymin, &Xmin, &Ymax, &Xmax);
	printf("Minimum and Maximum Locations X0: %d // X1: %d // Y0: %d // Y1: %d\n", Xmin, Xmax,Ymin, Ymax);
}

void printClusterCrops(const u8 *imageRGB, u8 *crop, int unique_mass_centers[MAX_CLUSTERS][2],
		int min_max_coordinates[MAX_CLUSTERS][4], u8 *resized_image,
		int dimensions[MAX_CLUSTERS][2],int console_height, int console_width, u8 sizing_factor){
	/*
		Print the crops of the detected clusters
	*/
	u16 X0, Y0, X1, Y1, box;
	box=112;

	for (int i = 0; i < MAX_CLUSTERS; i++) {
		if (min_max_coordinates[i][0] == 0 && min_max_coordinates[i][1] == 0 && min_max_coordinates[i][2] == 0 && min_max_coordinates[i][3] == 0) {
			break; // Stop printing when encountering an empty element
		}
			X0 = min_max_coordinates[i][2]*sizing_factor;
			Y0 = min_max_coordinates[i][0]*sizing_factor;
			X1 = min_max_coordinates[i][3]*sizing_factor;
			Y1 = min_max_coordinates[i][1]*sizing_factor;
			if (X1-X0>224 && Y1-Y0>224){
						if (X0<0 && X0>FRAME_WIDTH){
								X0=0;
						}
						if (X1>FRAME_WIDTH){
							X1=FRAME_WIDTH;
						}
						if (Y0<0 && Y0>FRAME_HEIGHT){
							Y0=0;
						}
						if (Y1>FRAME_HEIGHT){
							Y1=FRAME_HEIGHT;
						}
			crop_image(imageRGB, crop, FRAME_WIDTH, FRAME_HEIGHT, X0, Y0, X1, Y1);
			resize_image(crop, resized_image, X1-X0, Y1-Y0, console_width, console_height);
			image_on_console(resized_image, console_width, console_height);
			printf("Grouped Cluster %d: Min (X: %d, Y: %d), Max (X: %d, Y: %d)\n", i+1, X0, Y0, X1, Y1);
			dimensions [i][0]=X1-X0;
			dimensions [i][1]=Y1-Y0;
			printf("Dimensions: %d X %d \n", dimensions [i][0], dimensions [i][1]);
			}
			else{
				X0 = (unique_mass_centers[i][1]*sizing_factor) - box;
				Y0 = (unique_mass_centers[i][0]*sizing_factor) - box;
				X1 = (unique_mass_centers[i][1]*sizing_factor) + box;
				Y1 = (unique_mass_centers[i][0]*sizing_factor) + box;
						if (X0<0 && X0>FRAME_WIDTH){
							X0=0;
						}
						if (X1>FRAME_WIDTH){
							X1=FRAME_WIDTH;
						}
						if (Y0<0 && Y0>FRAME_HEIGHT){
							Y0=0;
						}
						if (Y1>FRAME_HEIGHT){
							Y1=FRAME_HEIGHT;
						}
			crop_image(imageRGB, crop, FRAME_WIDTH, FRAME_HEIGHT, X0, Y0, X1, Y1);
			resize_image(crop, resized_image, X1-X0, Y1-Y0, console_width, console_height);
			image_on_console(resized_image, console_width, console_height);
			printf("Grouped Cluster %d: Center of Mass at (X: %d, Y: %d)\n", i+1, sizing_factor*unique_mass_centers[i][1], sizing_factor*unique_mass_centers[i][0]);
			dimensions [i][0]=X1-X0;
			dimensions [i][1]=Y1-Y0;
			printf("Dimensions: %d X %d \n", dimensions [i][0], dimensions [i][1]);
			}
	}

}

void detectionDebug (const u8 *image, const u8 *background_image,const u8 *imageRGB, u8 *crop,
		int unique_mass_centers[MAX_CLUSTERS][2],  int min_max_coordinates[MAX_CLUSTERS][4],
		int grouped_clusters[MAX_CLUSTERS][MAX_LOCATIONS][2],int dimensions[MAX_CLUSTERS][2],
		u16 *locations, int *num_locations, u8 sizing_factor){
		/*
			Run the insect detection algorithm
		*/

		u8 new_image[FRAME_TOTAL_PIXELS/3];
		u8 bin_image[FRAME_TOTAL_PIXELS/3];
		u8 resized_mask[FRAME_TOTAL_PIXELS/3];
		u8 resized_image2[FRAME_TOTAL_PIXELS];

		//Detection Parameters
		int num = 10; // Variable for the detection function
		int countmax = 5; // Threshold for the detection function
		int img_width_resized=FRAME_WIDTH/sizing_factor;
		int img_height_resized=FRAME_HEIGHT/sizing_factor;
		int dist=10;
		int clusVar=50;
		int console_height = 44;
		int console_width = console_height * 16 / 9;

		background_subtraction(image, background_image, new_image, FRAME_WIDTH, FRAME_HEIGHT);
		mask_image(new_image, bin_image, FRAME_WIDTH, FRAME_HEIGHT);
		resize_mask(bin_image, resized_mask, FRAME_WIDTH, FRAME_HEIGHT, console_width, console_height);
		mask_on_console(resized_mask, console_width, console_height);
		binarize_image(bin_image, bin_image, FRAME_WIDTH, FRAME_HEIGHT);
		resize_mask(bin_image, resized_image2, FRAME_WIDTH, FRAME_HEIGHT, img_width_resized, img_height_resized);
		detectAndFilterLocations(resized_image2, img_width_resized, img_height_resized, num, countmax, locations, num_locations);
		printf("Detected locations (total: %d):\n", *num_locations);
		int idx[*num_locations];
		detectClusters(locations, *num_locations, dist, clusVar, idx, unique_mass_centers, grouped_clusters, min_max_coordinates);
		printClusterCrops(imageRGB, crop, unique_mass_centers, min_max_coordinates, resized_image2,
						dimensions, console_height, console_width, sizing_factor);
}


void MinMaxCrop(const u8 *imageRGB, u8 *crop, int unique_mass_centers[MAX_CLUSTERS][2],
		int min_max_coordinates[MAX_CLUSTERS][4], u8 *resized_image,
		int dimensions[MAX_CLUSTERS][2],int console_height, int console_width, u8 sizing_factor){
	/*
		Crop the image using the minimum and maximum values of the detected locations
	*/
	u16 X0, Y0, X1, Y1;
	printf("[INFO] Cropping using Minimum and Maximum Values only\n");

	for (int i = 0; i < MAX_CLUSTERS; i++) {
		if (min_max_coordinates[i][0] == 0 && min_max_coordinates[i][1] == 0 && min_max_coordinates[i][2] == 0 && min_max_coordinates[i][3] == 0) {
			break; // Stop printing when encountering an empty element
		}
			X0 = min_max_coordinates[i][2]*sizing_factor;
			Y0 = min_max_coordinates[i][0]*sizing_factor;
			X1 = min_max_coordinates[i][3]*sizing_factor;
			Y1 = min_max_coordinates[i][1]*sizing_factor;
						if (X0<0 && X0>FRAME_WIDTH){
								X0=0;
						}
						if (X1>FRAME_WIDTH){
							X1=FRAME_WIDTH;
						}
						if (Y0<0 && Y0>FRAME_HEIGHT){
							Y0=0;
						}
						if (Y1>FRAME_HEIGHT){
							Y1=FRAME_HEIGHT;
						}
			crop_image(imageRGB, crop, FRAME_WIDTH, FRAME_HEIGHT, X0, Y0, X1, Y1);
			resize_image(crop, resized_image, X1-X0, Y1-Y0, console_width, console_height);
			image_on_console(resized_image, console_width, console_height);
			printf("Grouped Cluster %d: Min (X: %d, Y: %d), Max (X: %d, Y: %d)\n", i+1, X0, Y0, X1, Y1);
			dimensions [i][0]=X1-X0;
			dimensions [i][1]=Y1-Y0;
			printf("Dimensions: %d X %d \n", dimensions [i][0], dimensions [i][1]);
		}
}

void MassCenterCrop(const u8 *imageRGB, u8 *crop, int unique_mass_centers[MAX_CLUSTERS][2],
		int min_max_coordinates[MAX_CLUSTERS][4], u8 *resized_image,
		int dimensions[MAX_CLUSTERS][2],int console_height, int console_width, u8 sizing_factor){
	/*
		Crop the image using the mass center values of the detected locations
	*/
	u16 X0, Y0, X1, Y1, box;
	box=112;
	printf("[INFO] Cropping using Mass Center Values only\n");


	for (int i = 0; i < MAX_CLUSTERS; i++) {
		if (min_max_coordinates[i][0] == 0 && min_max_coordinates[i][1] == 0 && min_max_coordinates[i][2] == 0 && min_max_coordinates[i][3] == 0) {
			break; // Stop printing when encountering an empty element
		}
		X0 = (unique_mass_centers[i][1]*sizing_factor) - box;
		Y0 = (unique_mass_centers[i][0]*sizing_factor) - box;
		X1 = (unique_mass_centers[i][1]*sizing_factor) + box;
		Y1 = (unique_mass_centers[i][0]*sizing_factor) + box;
			if (X0<0){
				X0=0;
				X1=2*box;
			}
			if (X1>FRAME_WIDTH){
				X1=FRAME_WIDTH;
				X0=FRAME_WIDTH-2*box;
			}
			if (Y0<0){
				Y0=0;
				Y1=2*box;
			}
			if (Y1>FRAME_HEIGHT){
				Y1=FRAME_HEIGHT;
				Y0=FRAME_HEIGHT-2*box;
			}
		crop_image(imageRGB, crop, FRAME_WIDTH, FRAME_HEIGHT, X0, Y0, X1, Y1);
		resize_image(crop, resized_image, X1-X0, Y1-Y0, IMAGE_WIDTH, IMAGE_HEIGHT);
		save_crop(imageRGB, FRAME_WIDTH*FRAME_HEIGHT*3, "ImageDoc1.bin");
		resize_image(crop, resized_image, X1-X0, Y1-Y0, console_width, console_height);
		image_on_console(resized_image, console_width, console_height);
		printf("Grouped Cluster %d: Center of Mass at (X: %d, Y: %d)\n", i+1, sizing_factor*unique_mass_centers[i][1], sizing_factor*unique_mass_centers[i][0]);
		dimensions [i][0]=X1-X0;
		dimensions [i][1]=Y1-Y0;
		printf("Dimensions: %d X %d \n", dimensions [i][0], dimensions [i][1]);
		}
	}
