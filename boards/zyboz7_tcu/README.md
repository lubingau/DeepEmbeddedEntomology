# Generating the hardware with Vivado

You can easily build the hardware with Vivado in batch mode:

```bash
vivado -mode batch -source tcu_zyboz7.tcl
```

Then open Vivado in gui mode, generate the bitstream, and export the hardware into a `.xsa` file.

Advice:
- Be sure to source Vivado before.
- This hardware has be built with Vivado 2022.2. It should works with other version, but be aware of updating process when opening the gui mode.
- Be aware to include the bitstream when you export the hardware.

# Running the evaluation with Vitis

You can easily run the application on Vitis by importing a Vitis project from a `.zip` file.

If you want to build the application from scratch, create a new application from the `.xsa` file, and then copy all the code into the `src/` directory. Don't forget to increase the heap size in `lscript.ld` and to include `xilffs` libraries in the BSP settings (see [Tensil documentation](https://www.tensil.ai/docs/tutorials/resnet20-zcu104/#tensil-for-vitis-embedded-applications)).

# Running your own model on the Zybo Z7

If you want to run your own model with your own dataset, you'll have to modify these files:
- `dataset.h`: Add the parameters and the path of your model and dataset. Be sure that you already generated your dataset into a binary file (see next section).
- `dataset.c`: Add the `classes` dictionnary with the label of your classes.
- `tensil_platform.h`: If you're running a bigger model than a ResNet50, or with a smaller architecture (so with a bigger `.tprog` file), maybe you'll need to increase the range of the differents buffers. Be aware that you're limited to 512MB on the Zybo Z7. For example, a ResNet50 compiled for a 4x4 systolic array need almost 1GB of memory to store the instruction from the `.tprog` file, so it's impossible to deploy on the Zybo Z7

# Generate your dataset into a binary file

Because we are working in bare-metal on the Zybo Z7, we choose to store the dataset into a binary file. It's easier to store, and very easy to read. Here is the structure of the binary file:

![binary dataset](binary_dataset.png)

To generate this file, simply compile and run:

```bash
g++ dataset_to_binary.cpp -o dataset_to_binary `pkg-config --cflags --libs opencv4`
./dataset_to_binary Tipu-12/test/ tipu12.bin 1
```
```bash
Usage: ./dataset_to_binary <dataset_path> <binary_file> <shuffle>
```

By default, the images with and height are set to 224x224. You can change that in the `#define` on top of the code.

If you want to reduce your dataset, if it's to big for the Zybo Z7 for example, you can uncomment the ***reduce_dataset*** function at the end of the main, and modify ***max_images*** to the number you want. Notice that all the binaries datasets on this repo are limited to 100 samples, except Cifar10.

Advice:
- Be aware to have already installed opencv on your computer.

# Read images from the Zybo Z7

All the images captured and saved on the Zybo are also into binary files. If there is more than 1 image into the binary file, if the images are stacked for example, it will find and save automatically all the images. Here is a code to read them on your own computer:

```bash
g++ binary_to_images.cpp -o binary_to_images `pkg-config --cflags --libs opencv4`
./binary_to_image image.bin . 1920 1080
```
```bash
Usage: ./binary_to_images <binary_file> <output_directory> <image_width> <image_height>
```

You can also read a binary dataset to debug:

```bash
g++ binary_to_dataset.cpp -o binary_to_dataset `pkg-config --cflags --libs opencv4`
./binary_to_dataset tipu.bin debug/ 224 224 5
```
```bash
Usage: ./binary_to_dataset <binary_file> <output_directory> <image_width> <image_height> <max_images>
