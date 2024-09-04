These scripts generate the hardware for each architecture and clock defined in the scripts with Vivado.
# WARNING
<span style="color: orange;">
These scripts are not ready to use directly. You need to modify the scripts to fit your project. </span>

# Generate the hardware and the software
The `generate_all.sh` script generates the hardware and the software for each clock and TCU architecture. The script can be executed with the following command:

```bash
sudo -E bash generate_all.sh
```

# Generate the hardware
If you want to only generate the hardware of a dedicated architecture with different clock, you can use the `generate_hardware.tcl` script. The script can be executed with the following command:

```bash
vivado -mode tcl -source generate_hardware.tcl -tclargs $project_name $array_size $design_bd_name $source_path
```
