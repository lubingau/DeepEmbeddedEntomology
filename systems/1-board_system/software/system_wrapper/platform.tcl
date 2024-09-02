# 
# Usage: To re-create this platform project launch xsct with below options.
# xsct /home/lubin/DeepEmbeddedEntomology/software/camera_tcu/system_wrapper/platform.tcl
# 
# OR launch xsct and run below command.
# source /home/lubin/DeepEmbeddedEntomology/software/camera_tcu/system_wrapper/platform.tcl
# 
# To create the platform in a different location, modify the -out option of "platform create" command.
# -out option specifies the output directory of the platform project.

platform create -name {system_wrapper}\
-hw {/home/lubin/DeepEmbeddedEntomology/hardware/camera_tcu/system_wrapper.xsa}\
-out {/home/lubin/DeepEmbeddedEntomology/software/camera_tcu}

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
platform active {system_wrapper}
platform config -updatehw {/home/lubin/DeepEmbeddedEntomology/hardware/camera_tcu/system_wrapper.xsa}
platform generate -domains 
platform config -updatehw {/home/lubin/DeepEmbeddedEntomology/hardware/camera_tcu/system_wrapper.xsa}
platform generate -domains 
platform generate -domains standalone_ps7_cortexa9_0 
platform active {system_wrapper}
platform config -updatehw {/home/lubin/DeepEmbeddedEntomology/hardware/camera_tcu/system_wrapper.xsa}
platform generate -domains 
platform config -updatehw {/home/lubin/DeepEmbeddedEntomology/hardware/camera_tcu/system_wrapper.xsa}
platform generate -domains 
platform active {system_wrapper}
platform config -updatehw {/home/lubin/DeepEmbeddedEntomology/hardware/camera_tcu/system_wrapper.xsa}
platform generate -domains 
platform config -updatehw {/home/lubin/DeepEmbeddedEntomology/hardware/camera_tcu/system_wrapper.xsa}
platform generate -domains 
platform config -updatehw {/home/lubin/DeepEmbeddedEntomology/hardware/camera_tcu/system_wrapper.xsa}
platform generate -domains 
platform active {system_wrapper}
platform config -updatehw {/home/lubin/DeepEmbeddedEntomology/hardware/camera_tcu/system_wrapper.xsa}
platform generate -domains 
platform clean
platform clean
platform generate
platform generate -domains standalone_ps7_cortexa9_0,zynq_fsbl 
platform generate -domains standalone_ps7_cortexa9_0,zynq_fsbl 
platform active {system_wrapper}
platform config -updatehw {/home/lubin/DeepEmbeddedEntomology/hardware/camera_tcu/system_wrapper.xsa}
platform generate -domains 
platform config -updatehw {/home/lubin/DeepEmbeddedEntomology/hardware/camera_tcu/system_wrapper_20.xsa}
platform generate -domains 
platform clean
platform generate
platform config -updatehw {/home/lubin/DeepEmbeddedEntomology/hardware/camera_tcu/system_wrapper_50.xsa}
platform generate -domains 
platform config -updatehw {/home/lubin/DeepEmbeddedEntomology/hardware/camera_tcu/system_wrapper_100.xsa}
platform generate -domains 
platform active {system_wrapper}
platform config -updatehw {/home/lubin/DeepEmbeddedEntomology/hardware/camera_tcu/system_wrapper_12_100M.xsa}
platform generate -domains standalone_ps7_cortexa9_0,zynq_fsbl 
platform config -updatehw {/home/lubin/DeepEmbeddedEntomology/hardware/camera_tcu/system_wrapper_12_50M.xsa}
platform generate -domains 
platform config -updatehw {/home/lubin/DeepEmbeddedEntomology/hardware/camera_tcu/system_wrapper_100M.xsa}
platform generate -domains 
platform active {system_wrapper}
platform config -updatehw {/home/lubin/DeepEmbeddedEntomology/hardware/camera_tcu/system_wrapper_100M.xsa}
platform generate -domains 
platform config -updatehw {/home/lubin/DeepEmbeddedEntomology/hardware/camera_tcu/system_wrapper_50M.xsa}
platform generate -domains 
platform generate
platform active {system_wrapper}
platform config -updatehw {C:/camera_detect_tcu/hardware/camera_detect_tcu/system_wrapper.xsa}
platform generate
platform clean
platform clean
platform clean
platform generate
platform generate
platform clean
platform generate
platform active {system_wrapper}
platform config -updatehw {D:/David/system_wrapper.xsa}
platform generate -domains 
platform config -updatehw {C:/camera_detect_tcu/hardware/camera_detect_tcu/system_wrapper.xsa}
platform generate -domains 
platform clean
platform generate
platform generate
platform active {system_wrapper}
platform config -updatehw {/home/lubin/DeepEmbeddedEntomology/hardware/camera_detect_tcu/camera_detect_tcu/system_wrapper_100M.xsa}
platform generate -domains 
platform active {system_wrapper}
bsp reload
platform config -remove-boot-bsp
platform write
platform config -create-boot-bsp
platform write
bsp reload
domain active {zynq_fsbl}
bsp reload
platform generate -domains standalone_ps7_cortexa9_0,zynq_fsbl 
platform active {system_wrapper}
platform config -updatehw {/home/lubin/DeepEmbeddedEntomology/hardware/camera_detect_tcu/camera_detect_tcu/system_wrapper.xsa}
platform generate -domains standalone_ps7_cortexa9_0,zynq_fsbl 
platform generate
platform active {system_wrapper}
platform config -updatehw {/home/lubin/DeepEmbeddedEntomology/hardware/camera_detect_tcu/camera_detect_tcu/system_wrapper.xsa}
platform generate -domains 
domain active {standalone_ps7_cortexa9_0}
bsp reload
domain active {zynq_fsbl}
bsp reload
bsp reload
platform generate -domains 
platform active {system_wrapper}
platform generate -domains 
platform generate
