# 
# Usage: To re-create this platform project launch xsct with below options.
# xsct /home/lubin/DeepEmbeddedEntomology/software/camera_detect/system_wrapper/platform.tcl
# 
# OR launch xsct and run below command.
# source /home/lubin/DeepEmbeddedEntomology/software/camera_detect/system_wrapper/platform.tcl
# 
# To create the platform in a different location, modify the -out option of "platform create" command.
# -out option specifies the output directory of the platform project.

platform create -name {system_wrapper}\
-hw {/home/lubin/DeepEmbeddedEntomology/hardware/camera_detect_tcu/camera_detect_tcu/system_wrapper.xsa}\
-out {/home/lubin/DeepEmbeddedEntomology/software/camera_detect}

platform write
domain create -name {standalone_ps7_cortexa9_0} -display-name {standalone_ps7_cortexa9_0} -os {standalone} -proc {ps7_cortexa9_0} -runtime {cpp} -arch {32-bit} -support-app {empty_application}
platform generate -domains 
platform active {system_wrapper}
domain active {zynq_fsbl}
domain active {standalone_ps7_cortexa9_0}
platform generate -quick
bsp reload
bsp setlib -name xilffs -ver 4.8
bsp config use_lfn "1"
bsp write
bsp reload
catch {bsp regenerate}
platform generate
platform config -updatehw {/home/lubin/DeepEmbeddedEntomology/hardware/camera_detect/system_wrapper.xsa}
platform generate -domains 
platform config -updatehw {/home/lubin/DeepEmbeddedEntomology/hardware/camera_detect/system_wrapper.xsa}
platform generate -domains 
