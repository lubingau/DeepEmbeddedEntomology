#!/bin/bash

# Example: bash extract.sh dram1_depth_4194304/4/resnet50v2_imagenet

if [ "$#" -ne 1 ]; then
    echo "Usage: bash extract.sh <files_dir>"
    exit 1
fi
files_dir=$1


power_file=$files_dir/power.txt
utilization_file=$files_dir/utilization.txt
timing_file=$files_dir/timing.txt

# POWER
line=$(sed -n '36p' "$power_file")
# Search the first number with 5 digits
power=$(echo "$line" | grep -o '[0-9].\{5\}')
# Remove all the spaces
power=$(echo $power | tr -d ' ')
echo "Power: $power W"


# UTILIZATION
LUT_line=$(sed -n '36p' "$utilization_file")
LUTRAM_line=$(sed -n '37p' "$utilization_file")
FF_line=$(sed -n '41p' "$utilization_file")
BRAM_line=$(sed -n '106p' "$utilization_file")
DSP_line=$(sed -n '121p' "$utilization_file")

extract_line_infos() {
    local line="$2"
    # Remove all the spaces
    line=$(echo "$line" | tr -d ' ')
    # Split the line with the | character
    IFS='|' read -ra fields <<< "$line"
    local used="${fields[2]}"
    local total="${fields[5]}"
    local percentage="${fields[6]}"
    echo "$1: $used / $total ($percentage %)"
}

extract_line_infos "LUT" "$LUT_line"
extract_line_infos "LUTRAM" "$LUTRAM_line"
extract_line_infos "FF" "$FF_line"
extract_line_infos "BRAM" "$BRAM_line"
extract_line_infos "DSP" "$DSP_line"

# TIMING
timing_line=$(sed -n '139p' "$timing_file")
IFS=' ' read -ra fields <<< "$timing_line"
WNS="${fields[0]}"
TNS="${fields[1]}"
TNS_failing_endpoints="${fields[2]}"
TNS_total_endpoints="${fields[3]}"
echo "WNS(ns): $WNS TNS(ns): $TNS  Failing endpoints: $TNS_failing_endpoints  Total endpoints: $TNS_total_endpoints"