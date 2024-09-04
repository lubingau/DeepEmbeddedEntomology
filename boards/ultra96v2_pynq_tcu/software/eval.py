import sys
import os
import time
import numpy as np
import pynq
from pynq import Overlay
from tcu_pynq.driver import Driver
from tcu_pynq.architecture import ultra96
import matplotlib.pyplot as plt
from PIL import Image


def show_img(data, n):
    img_pil = Image.fromarray(data[n])
    plt.imshow(img_pil)

def preprocessing(data):
    # print(data[n].shape)
    if len(data.shape) > 2 and data.shape[2] > 3:
        data = data[:, :, :3]
    data_norm = data.astype('float32') / 255.0
    mean = np.array([0.485, 0.456, 0.406])
    std = np.array([0.229, 0.224, 0.225])
    data_norm = data_norm - mean
    data_norm = data_norm / std
    # img = np.transpose(data_norm.reshape((3, 224, 224)), axes=[1, 2, 0])
    data_norm = np.pad(data_norm, [(0, 0), (0, 0), (0, tcu.arch.array_size - 3)], 'constant', constant_values=0)
    return data_norm.reshape((-1, tcu.arch.array_size))

def load_test_data(test_dir, max_images=20):
    data = []
    labels = []
    label_names = sorted(os.listdir(test_dir))  # Get class names from folder names

    for label_idx, label_name in enumerate(label_names):
        if len(data) >= max_images:
            break
        label_dir = os.path.join(test_dir, label_name)
        for img_name in os.listdir(label_dir):
            img_path = os.path.join(label_dir, img_name)
            img = Image.open(img_path)
            img = img.resize((224, 224))
            img_array = np.array(img)
            img_array.astype(np.uint8)

            data.append(img_array)
            labels.append(label_idx)

            if len(data) >= max_images:
                break

        print(f"Loaded {label_name} images")

    print(f"Loaded {len(data)} images in total")
    
    return data, labels, label_names

def shuffle_data(data, labels):
    labels = np.array(labels)
    idx = np.random.permutation(len(data))
    new_data = data[idx]
    new_labels = labels[idx]
    np.save('shuffled_data.npy', new_data)
    np.save('shuffled_labels.npy', new_labels)
    print("Data shuffled and saved")

def load_float_batch(data, n, batch_size):
    batch = []
    for i in range(batch_size):
        img = preprocessing(data, n + i)
        batch.append(img)
    return np.array(batch)

def print_progress_bar(label, percentage, max_length=50):
    filled_length = int(max_length * percentage // 100)
    bar = '#' * filled_length + '-' * (max_length - filled_length)
    # define 11 spaces for the label
    return f'{label[:11]:<11} [{bar}] {percentage:.2f}%'

if __name__ == '__main__':
    # Needed to run inference on TCU
    sys.path.append('/home/xilinx')

    # Load the overlay and driver
    overlay = Overlay('/home/xilinx/hardware/tcu_ultra96.bit')
    tcu = Driver(ultra96, overlay.axi_dma_0)
    print("Loaded overlay and driver")

    # Load the model
    tcu.load_model('/home/xilinx/model/resnet50_tipu12_onnx_ultra96v2.tmodel')
    N_CLASSES = 12
    print("Loaded model")

    # Load the data
    test_dir = '/home/xilinx/test'
    print(f"Loading test data from {test_dir}...")
    data, labels, label_names = load_test_data(test_dir, 10000000)
    # shuffle_data(data, labels)

    # Run evaluation
    results = []
    total_time = 0
    tcu_time = 0
    for n in range(0, len(data)):
        start = time.time()
        img = preprocessing(data[n])
        inputs = {'x:0': img}
        start_tcu = time.time()
        outputs = tcu.run(inputs)
        classes = outputs['Identity:0'][:N_CLASSES]
        result_idx = np.argmax(classes)
        results.append(result_idx)
        end = time.time()
        total_time += end - start
        tcu_time += end - start_tcu
        if n % 50 == 0 and n > 0:
            print(f"Processed {n} images")
    print("Evaluation complete")
    
    # Calculate FPS
    fps = len(data) / total_time
    fps_tcu = len(data) / tcu_time
    print(f"Total execution time: {total_time:.2f} seconds ({total_time / 60:.2f} minutes)")
    print(f"TCU execution time: {tcu_time:.2f} seconds ({tcu_time / 60:.2f} minutes)")
    print(f"Total FPS: {fps:.2f}")
    print(f"TCU FPS: {fps_tcu:.2f}")

    # Calculate accuracy per class
    correct = 0
    class_correct = np.zeros(N_CLASSES)
    for i in range(len(data)):
        if results[i] == labels[i]:
            correct += 1
            class_correct[labels[i]] += 1
    
    global_accuracy = correct / len(data)
    class_counts = np.bincount(labels)
    if len(class_counts) < N_CLASSES:
        class_counts = np.pad(class_counts, (0, N_CLASSES - len(class_counts)), 'constant', constant_values=0) + 1e-6
    
    class_accuracies = class_correct / class_counts

    for i in range(N_CLASSES):
        print("Class", print_progress_bar(label_names[i], class_accuracies[i] * 100))

    print(f"Global accuracy: {100.0*global_accuracy:.2f}%")


        


