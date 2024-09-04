#!/bin/sh

# Copyright © 2023 Advanced Micro Devices, Inc. All rights reserved.
# SPDX-License-Identifier: MIT

## Author: Daniele Bagni, AMD/Xilinx Inc
## date 26 May 2023


if [ $1 = kv260 ]; then
      ARCH=/workspace/arch/arch_kria_kv260.json
      TARGET=kv260
      echo "-----------------------------------------"
      echo "COMPILING MODEL FOR KV260.."
      echo "-----------------------------------------"
elif [ $1 = ultra96v2 ]; then
      ARCH=/workspace/arch/arch_ultra.json
      TARGET=ultra96v2
      echo "-----------------------------------------"
      echo "COMPILING MODEL FOR Ultra96v2.."
      echo "-----------------------------------------"
else
      echo  "Target not found. Valid choices are: zcu102, zcu104, vck190, vck5000, vek280, v70 ...exiting"
      exit 1
fi

CNN_MODEL=$2

compile() {
  vai_c_xir \
	--xmodel           build/quantized/${CNN_MODEL} \
	--arch            ${ARCH} \
	--output_dir      build/compiled_${TARGET} \
	--net_name        ${TARGET}_${CNN_MODEL}
#	--options         "{'mode':'debug'}"
#  --options         '{"input_shape": "1,224,224,3"}' \
}

compile #2>&1 | tee build/log/compile_$TARGET.log

echo "-----------------------------------------"
echo "MODEL COMPILED"
echo "-----------------------------------------"
