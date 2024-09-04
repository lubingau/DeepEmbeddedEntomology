# Usage with Vitis IDE:
# In Vitis IDE create a Single Application Debug launch configuration,
# change the debug type to 'Attach to running target' and provide this 
# tcl script in 'Execute Script' option.
# Path of this script: /home/lubin/DeepEmbeddedEntomology/software/camera_detect/camera_detect_system/_ide/scripts/debugger_camera_detect-default.tcl
# 
# 
# Usage with xsct:
# To debug using xsct, launch xsct and run below command
# source /home/lubin/DeepEmbeddedEntomology/software/camera_detect/camera_detect_system/_ide/scripts/debugger_camera_detect-default.tcl
# 
connect -url tcp:127.0.0.1:3121
targets -set -nocase -filter {name =~"APU*"}
rst -system
after 3000
targets -set -filter {jtag_cable_name =~ "Digilent Zybo Z7 210351B7C158A" && level==0 && jtag_device_ctx=="jsn-Zybo Z7-210351B7C158A-23727093-0"}
fpga -file /home/lubin/DeepEmbeddedEntomology/software/camera_detect/camera_detect/_ide/bitstream/system_wrapper.bit
targets -set -nocase -filter {name =~"APU*"}
loadhw -hw /home/lubin/DeepEmbeddedEntomology/software/camera_detect/system_wrapper/export/system_wrapper/hw/system_wrapper.xsa -mem-ranges [list {0x40000000 0xbfffffff}] -regs
configparams force-mem-access 1
targets -set -nocase -filter {name =~"APU*"}
source /home/lubin/DeepEmbeddedEntomology/software/camera_detect/camera_detect/_ide/psinit/ps7_init.tcl
ps7_init
ps7_post_config
targets -set -nocase -filter {name =~ "*A9*#0"}
dow /home/lubin/DeepEmbeddedEntomology/software/camera_detect/camera_detect/Debug/camera_detect.elf
configparams force-mem-access 0
targets -set -nocase -filter {name =~ "*A9*#0"}
con