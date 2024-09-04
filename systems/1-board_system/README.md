# Generating the hardware with Vivado

You can easily build the hardware with Vivado in batch mode:

```bash
vivado -mode batch -source camera_detect_tcu.tcl
```

Then open Vivado in gui mode, generate the bitstream, and export the hardware into a `.xsa` file.

Advice:
- Be sure to source Vivado before.
- This hardware has be built with Vivado 2022.2. It should works with other version, but be aware of updating process when opening the gui mode.
- Be aware to include the bitstream when you export the hardware.

# Running the evaluation with Vitis

You can easily run the application on Vitis by importing a Vitis project from a `.zip` file.

If you want to build the application from scratch, create a new application from the `.xsa` file, and then copy all the code into the `src/` directory. Don't forget to increase the heap size in `lscript.ld` and to include `xilffs` libraries in the BSP settings (see [Tensil documentation](https://www.tensil.ai/docs/tutorials/resnet20-zcu104/#tensil-for-vitis-embedded-applications)).