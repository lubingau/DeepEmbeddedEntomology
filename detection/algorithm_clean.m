clear all
clc

% Varios insectos: 5, 10, 20 
% Imagen 21, 26: Necesario Filtro Previo

% Load and preprocess the image
A = imread("10.jpeg");
A = rgb2gray(A);
[m, n] = size(A); % m-row & n-col
Ad = double(A);

B = imread("background.jpeg");
B = rgb2gray(B);
Bd = double(B);

% Background Subtraction
C_sub = Ad - Bd + 100;

% Binarization
threshold = 50;
A_bi = uint8(C_sub > threshold) * 255; 


% Display images
figure (1);
subplot(2, 2, 1);
imshow(A);  
title('Original Image');

subplot(2, 2, 2);
imshow(B);  
title('Background Image');

subplot(2, 2, 3);
imshow(C_sub, []);  
title('Subtracted Image');

subplot(2, 2, 4);
imshow(A_bi);  
title('Binarized Image');


% Binarization
Bin = A_bi ~= 255;

% Detection Algorithm
num = 7;
countmax = 3;
locations = detectLocations(Bin, m, n, num, countmax);


k = size(locations, 1);
C = 1;   % Counter for clusters
idx = zeros(k, 1); % Initialize cluster indices

% Assign initial unique labels to each point
for i = 1:k
    idx(i) = C;
    C = C + 1;
end

dist = 5; % Distance threshold
usedlabels = [];

% Clustering based on distance
for i = 1:k
    for j = 1:k
        if i ~= j % Skip comparing a point with itself
            Xdiff = abs(locations(j, 1) - locations(i, 1));
            Ydiff = abs(locations(j, 2) - locations(i, 2));
            if Xdiff < dist && Ydiff < dist
                if ~ismember(idx(j), usedlabels)
                    idx(j) = idx(i); % Assign the same cluster label
                    usedlabels = [usedlabels, idx(i)];
                end
            end
        end
    end
end

% Calculate mass centers for each cluster
unique_clusters = unique(idx);
mass_centers = zeros(length(unique_clusters), 2); % Initialize mass centers array

for i = 1:length(unique_clusters)
    cluster_label = unique_clusters(i);
    cluster_points = locations(idx == cluster_label, :);
    [Xm, Ym] = massCenter(cluster_points);
    mass_centers(i, :) = [Xm, Ym];
%    fprintf('Cluster %d: Center of Mass at (X: %f, Y: %f)\n', cluster_label, Xm, Ym);
end

% Group clusters with close mass centers
clusVar = 50;
box=112;
unique_mass_centers = [];
used_indices = [];
grouped_clusters = cell(0); % Initialize cell array for grouped clusters
min_max_coordinates = []; % Initialize array for min and max coordinates

for i = 1:size(mass_centers, 1)
    if ismember(i, used_indices)
        continue;
    end
    
    close_indices = i;
    for j = i+1:size(mass_centers, 1)
        a = abs(mass_centers(j, 1) - mass_centers(i, 1));
        b = abs(mass_centers(j, 2) - mass_centers(i, 2));
        if a < clusVar && b < clusVar
            close_indices = [close_indices, j];
            used_indices = [used_indices, j];
        end
    end
    
    % Calculate the mean mass center of grouped clusters
    grouped_mass_center = mean(mass_centers(close_indices, :), 1);
    unique_mass_centers = [unique_mass_centers; grouped_mass_center];
    
    % Aggregate points from the grouped clusters
    cluster_points = vertcat(locations(ismember(idx, unique_clusters(close_indices)), :));
    grouped_clusters{end+1} = cluster_points; % Add grouped cluster points to the cell array
    
    % Calculate min and max coordinates for the grouped cluster
    Xmin = min(cluster_points(:, 1));
    Xmax = max(cluster_points(:, 1));
    Ymin = min(cluster_points(:, 2));
    Ymax = max(cluster_points(:, 2));
    min_max_coordinates = [min_max_coordinates; Xmin, Xmax, Ymin, Ymax];

    % Assign new labels to the grouped clusters
    new_label = length(grouped_clusters); % New label for this grouped cluster
    for k = 1:length(close_indices)
        new_idx(idx == unique_clusters(close_indices(k))) = new_label;
    end

    fprintf('Grouped Cluster %d: Center of Mass at (X: %f, Y: %f)\n', i, uint16 (grouped_mass_center(1)), uint16 (grouped_mass_center(2)));
    fprintf('Grouped Cluster %d: Min (X: %f, Y: %f), Max (X: %f, Y: %f)\n', i, Xmin, Ymin, Xmax, Ymax);
        
        if (Xmax-Xmin>box && Ymax-Ymin>box)
        
        % Define the rectangle for cropping: [xmin, ymin, width, height]
            crop_rect = [Xmin, Ymin, Xmax - Xmin + 1, Ymax - Ymin + 1];
            
            % Crop the image
            cropped_image1 = imcrop(A, crop_rect);
            
            % Display the cropped image
            figure;
            imshow(cropped_image1);
            title(sprintf('Cropped Image Min and Max %d', i));

        else
            Xm = grouped_mass_center(1);
            Ym = grouped_mass_center(2);
            cropped_image = cropImageCenter(A, Xm, Ym, box);
            
            % Display the cropped image
            figure;
            imshow(cropped_image);
            title(sprintf('Cropped Image Mass Center %d', i));
        end
end

figure (3);
gscatter(locations(:,1),-locations(:,2),new_idx);
title('Clustering')



%%
function locations = detectLocations(Bin, m, n, num, countmax)
    locations = [];
    for i = 1:m
        for j = 1:n
            if Bin(i, j) == 1 && j + num <= n && i + num <= m 
                count_h = sum(Bin(i, j+1:j+num));
                count_v = sum(Bin(i+1:i+num, j));
                if count_h > countmax && + count_v>countmax;
                    locations = [locations; j, i];
                end
            end
        end
    end
end

function [Xm, Ym] = massCenter(locations)
    
    sumX = sum(locations(:, 1));
    sumY = sum(locations(:, 2));
    
    numPoints = size(locations, 1);
    
    Xm = sumX / numPoints;
    Ym = sumY / numPoints;
    
end

function cropped_image = cropImageCenter(A, Xm, Ym, box)
    
    width = box * 2;
    height = box * 2;
    
    % Calculate crop boundaries
    Xmin_crop = max(1, round(Xm - width / 2));
    Ymin_crop = max(1, round(Ym - height / 2));
    Xmax_crop = min(size(A, 2), round(Xm + width / 2 - 1));
    Ymax_crop = min(size(A, 1), round(Ym + height / 2 - 1));
    
    % Ensure crop region is within bounds
    if Xmax_crop > size(A, 2)
        Xmax_crop = size(A, 2);
        if (Xmin_crop>(size(A, 2)-box))
            Xmin_crop=size(A, 2)-box
        end
    end
    if Ymax_crop > size(A, 1)
        Ymax_crop = size(A, 1);
        if (Ymin_crop>(size(A, 1)-box))
            Ymin_crop=size(A, 1)-box
        end
    end
    
    % Crop the image
    cropped_image = A(Ymin_crop:Ymax_crop, Xmin_crop:Xmax_crop);
end
