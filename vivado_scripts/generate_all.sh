#!/bin/bash | sudo -E bash hardware/generate_all.sh | execute from the DeepEmbeddedEntomology directory
start=$(date +%s)

input_model=resnet50_bee306_best.onnx
model_name=$(basename "$input_model" .onnx)
arch_file=zyboz7.tarch
output_dir=dram1_depth_4194304
project_name=tcu_zyboz7
design_bd_name=tcu_zyboz7
mkdir -p $output_dir

array_size_list=(8 12)
######################### TENSIL COMPILATION #########################
echo " "
echo "-------------------------------------------------------------------------------------------------------------------------------------------------"
echo "Compile architectures for different array sizes"
echo "-------------------------------------------------------------------------------------------------------------------------------------------------"
echo " "
for array_size in "${array_size_list[@]}"
do
    echo " "
    echo "-------------------------------------------------------------------------------------------------------------------------------------------------"
    echo "Compile architecture for array size: $array_size"
    echo "-------------------------------------------------------------------------------------------------------------------------------------------------"
    echo " "
    mkdir -p $output_dir/$array_size
    output_path=$output_dir/$array_size/$model_name/
    output_path_rtl=$output_dir/$array_size/
    # Modify the array size in the architecture file
    sed -i 's/"array_size": 8/"array_size": '$array_size'/g' $arch_file
    # Compile the architecture with Tensil
    sudo -E python3 /home/lubin/DeepEmbeddedEntomology/onnx_to_tensil.py -o $input_model -d $output_path -a $arch_file --no-rtl
    # Generate the RTL files with Tensil
    sudo -E python3 /home/lubin/DeepEmbeddedEntomology/arch_to_tensil.py -a $arch_file -d $output_path_rtl
    # Reset the array size to 8
    sed -i 's/"array_size": '$array_size'/"array_size": 8/g' $arch_file
    # Change the owner of the generated files
    sudo chown -R lubin *
    # Delete useless files
    find $output_path_rtl -type f -not \( -name "*.v" -o -name "*.log" -o -name "*.h" -o -name "*.t*" -o -name "*.xsa" \) -exec rm {} +
done

# ######################### VIVADO GENERATION #########################
# echo " "
# echo "-------------------------------------------------------------------------------------------------------------------------------------------------"
# echo "Generate hardware for different architectures"
# echo "-------------------------------------------------------------------------------------------------------------------------------------------------"
# echo " "
# source /home/lubin/vivado2022.sh
# for array_size in "${array_size_list[@]}"
# do
#     echo " "
#     echo "-------------------------------------------------------------------------------------------------------------------------------------------------"
#     echo "Generate hardware for array size: $array_size"
#     echo "-------------------------------------------------------------------------------------------------------------------------------------------------"
#     echo " "
#     # Save pwd or output_dir into the variable tmp
#     tmp=$PWD
#     source_path=$tmp/$output_dir/$array_size
#     vivado -mode tcl -source /home/lubin/DeepEmbeddedEntomology/hardware/generate_hardware.tcl -tclargs $project_name $array_size $design_bd_name $source_path
# done

# Change the owner of the generated files
sudo chown -R lubin *

elapsed_min=$(echo "scale=2;($(date +%s)-$start)/60" | bc)
echo " "
echo "-------------------------------------------------------------------------------------------------------------------------------------------------"
echo "Finish in $elapsed_min minutes"
echo "-------------------------------------------------------------------------------------------------------------------------------------------------"
echo " "
