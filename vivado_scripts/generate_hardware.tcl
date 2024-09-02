
if {$::argc != 4} {
    puts "Usage: vivado -mode tcl -source generate_platforms.tcl -tclargs project_name array_size design_bd_name source_dir"
    exit
}

set project_name [lindex $::argv 0]
set array_size [lindex $::argv 1]
set design_bd_name [lindex $::argv 2]
set source_dir [lindex $::argv 3]

puts "-------------------------------------------------------------------------------------------------------------------------------------------------\nGenerating hardware platforms for different clocks with the following options:\n\n------ Project name: $project_name\n------ Array size: $array_size\n------ Design BD name: $design_bd_name\n------ Source directory: $source_dir\n-------------------------------------------------------------------------------------------------------------------------------------------------\n"

# Consider that all the projects are in the same directory (hardware/)
set project_path /home/lubin/DeepEmbeddedEntomology/hardware/$project_name
open_project $project_path/$project_name.xpr
update_compile_order -fileset sources_1
# Open the block design
open_bd_design $project_path/$project_name.srcs/sources_1/bd/$design_bd_name/$design_bd_name.bd

# Define a base folder for the original files
set imports_dir /home/lubin/DeepEmbeddedEntomology/hardware/$project_name/$project_name.srcs/sources_1/imports/hardware
set base_dir /home/lubin/DeepEmbeddedEntomology/hardware
set data_type 16
set buffer_size [expr {$data_type * $array_size}]
# Update the architecture files
puts "\n-------------------------------------------------------------------------------------------------------------------------------------------------\nUpdating architecture files for array size $array_size\n-------------------------------------------------------------------------------------------------------------------------------------------------\n"

update_files -from_files $source_dir/bram_dp_${buffer_size}x2048.v -filesets [get_filesets *] -to_files $imports_dir/bram_dp_128x2048.v
update_files -from_files $source_dir/bram_dp_${buffer_size}x8192.v -filesets [get_filesets *] -to_files $imports_dir/bram_dp_128x8192.v
update_files -from_files $source_dir/top_zyboz7.v -filesets [get_filesets *] -to_files $imports_dir/top_zyboz7.v
update_compile_order -fileset sources_1
update_module_reference tcu_zyboz7_top_zyboz7_0_3
update_compile_order -fileset sources_1

# Define clocks
set clocks_list {50 75 100 125}

# Generate bitstream and report power, timing, and utilization for each clock
foreach clock $clocks_list {
    puts " "
    puts "-------------------------------------------------------------------------------------------------------------------------------------------------"
    puts "Clock: $clock"
    puts "-------------------------------------------------------------------------------------------------------------------------------------------------"
    puts " "
    set output_path $source_dir/${clock}MHz
    file mkdir $output_path
    # Set clock
    startgroup
    set_property CONFIG.PCW_FPGA0_PERIPHERAL_FREQMHZ $clock [get_bd_cells processing_system7_0]
    endgroup
    update_compile_order -fileset sources_1
    make_wrapper -files [get_files $project_path/$project_name.srcs/sources_1/bd/$design_bd_name/$design_bd_name.bd] -top
    # Generate bitstream
    reset_runs impl_1
    launch_runs impl_1 -to_step write_bitstream -jobs 3
    wait_on_run impl_1
    # Report power, timing, and utilization and save hardware platform
    open_run impl_1
    report_power -file $output_path/power.txt
    report_timing_summary -file $output_path/timing.txt
    report_utilization -spreadsheet_table Summary -file $output_path/utilization.txt
    write_hw_platform -fixed -include_bit -force -file $output_path/${design_bd_name}_wrapper.xsa
}

# Reinitialize clock to 50 MHz
startgroup
set_property CONFIG.PCW_FPGA0_PERIPHERAL_FREQMHZ {50} [get_bd_cells processing_system7_0]
endgroup
# Reinitialize architecture files to the original ones from the base folder
puts "\n-------------------------------------------------------------------------------------------------------------------------------------------------\nReinitializing architecture files\n-------------------------------------------------------------------------------------------------------------------------------------------------\n"

update_files -from_files $base_dir/bram_dp_128x2048.v -to_files $imports_dir/bram_dp_${buffer_size}x2048.v -filesets [get_filesets *]
update_files -from_files $base_dir/bram_dp_128x8192.v -to_files $imports_dir/bram_dp_${buffer_size}x8192.v -filesets [get_filesets *]
update_files -from_files $base_dir/top_zyboz7.v -to_files $imports_dir/top_zyboz7.v -filesets [get_filesets *]
update_compile_order -fileset sources_1
update_module_reference tcu_zyboz7_top_zyboz7_0_3
update_compile_order -fileset sources_1
make_wrapper -files [get_files $project_path/$project_name.srcs/sources_1/bd/$design_bd_name/$design_bd_name.bd] -top

save_bd_design
close_project
exit

