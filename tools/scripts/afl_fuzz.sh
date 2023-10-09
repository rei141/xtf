#!/bin/bash
set -e

export AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=1
export AFL_SKIP_CPUFREQ=1
export AFL_DISABLE_TRIM=1
export AFL_INST_RATIO=0
export AFL_AUTORESUME=1
export AFL_FAST_CAL=1
CONFIG_PATH="./config.yaml"  # default path
AFL_DIR=""
MODE=""
ID=""
OUT_DIR=""
OUT=""

show_usage() {
    echo "Usage: sudo afl_fuzz [-o output] [-c config_path] [-m mode] [-i id] [-h help]"
    echo ""
    echo "Arguments:"
    echo "-o output       : Directory where AFL will write its output."
    echo "-c config_path  : Path to configuration file. Default is ./config.yaml"
    echo "-m mode         : The operation mode. Options are 'c', 'M', or 'S'. Both mode and id must be specified together."
    echo "-i id           : An ID for the fuzzing process. It's used in the creation of shared memory segments and other identifiers. Both mode and id must be specified together."
    echo "-h help         : Display this help message."
    echo ""
    echo "Example:"
    echo "  afl_fuzz.sh -o output_dir -m M -i 1"
    echo "  This command will run the fuzzing job as the master process and output results to the 'output_dir' directory."
}

check_file() {
    if [ ! -f "$1" ]; then
        echo "Error: File $1 does not exist."
        exit 1
    fi
}

while getopts ":o:c:m:i:h" opt; do
  case ${opt} in
    o ) OUT=$OPTARG ;;
    c ) CONFIG_PATH=$OPTARG ;;  # config path
    m ) MODE=$OPTARG ;;
    i ) ID=$OPTARG ;;
    h ) show_usage; exit 0 ;;
    \? ) show_usage; exit 1 ;;
  esac
done
if [ -z "$OUT" ]
then
    echo "Output option (-o) is required."
    show_usage
    exit 1
fi
if [ \( -n "$MODE" -a -z "$ID" \) -o \( -z "$MODE" -a -n "$ID" \) ]
then
    echo "Both mode (-m) and id (-i) must be specified together."
    show_usage
    exit 1
fi

check_file $CONFIG_PATH

AFL_DIR=$(python3 ./tools/scripts/get_yaml.py $CONFIG_PATH directories afl_dir)

check_file "$AFL_DIR/afl-gcc"
check_file "$AFL_DIR/afl-fuzz"

WORK_DIR=$(python3 ./tools/scripts/get_yaml.py $CONFIG_PATH directories work_dir)

$AFL_DIR/afl-gcc -o $WORK_DIR/tools/fuzz_auto $WORK_DIR/tools/src/fuzz_auto.c -I$WORK_DIR/tools/include -lelf -lyaml

check_file "$WORK_DIR/tools/fuzz_auto"

if [ "$MODE" = "c" ]; then
    $WORK_DIR/tools/qemu_server "ivmshm_$ID" "afl_bitmap_$ID" > /dev/null 2>&1 &
    $AFL_DIR/afl-fuzz -i- -o "$OUT"/ -g 4096 -G 4096 -f afl_input -t 15000 -s 7 $WORK_DIR/tools/fuzz_auto "ivmshm_$ID" "afl_bitmap_$ID"
elif [ "$MODE" = "M"  ]; then
    $WORK_DIR/tools/qemu_server "ivmshm_$ID" "afl_bitmap_$ID" > /dev/null 2>&1 &
    $AFL_DIR/afl-fuzz -i ./random_input -o "$OUT" -M fuzzer$ID -g 4096 -G 4096 -f afl_input_$ID -t 15000 -s 7 $WORK_DIR/tools/fuzz_auto "ivmshm_$ID" "afl_bitmap_$ID" afl_input_$ID
elif [ "$MODE" = "S"  ]; then
    $WORK_DIR/tools/qemu_server "ivmshm_$ID" "afl_bitmap_$ID" > /dev/null 2>&1 &
    $AFL_DIR/afl-fuzz -i ./random_input -o "$OUT" -S fuzzer$ID -g 4096 -G 4096 -f afl_input_$ID -t 15000 -s 7 $WORK_DIR/tools/fuzz_auto "ivmshm_$ID" "afl_bitmap_$ID" afl_input_$ID
elif [ -z $MODE ]; then
    $AFL_DIR/afl-fuzz -i ./random_input_0 -o "$OUT" -g 4096 -G 4096 -f afl_input -t 6000 -s 7 $WORK_DIR/tools/fuzz_auto "ivmshm" "afl_bitmap" "-d" "tests/necofuzz/"

else
    echo "Usage: afl_fuzz.sh <output_directory> <operation> <fuzzer_id>"
    echo ""
    echo "This script runs AFLplusplus with various configuration options."
    echo ""
    echo "Arguments:"
    echo "<output_directory>  - Directory where AFL will write its output."
    echo "<operation>         - The operation mode. Options are 'c', 'M', or 'S'."
    echo "   'c' - Run the fuzzing job without additional coverage instrumentation and reporting."
    echo "   'M' - Run the fuzzing job as the master process in a multi-fuzzing setup."
    echo "   'S' - Run the fuzzing job as a secondary process in a multi-fuzzing setup."
    echo "<fuzzer_id>         - An ID for the fuzzing process. It's used in the creation of shared memory segments and other identifiers."
    echo ""
    echo "Example:"
    echo "  afl_fuzz.sh output_dir M 1"
    echo "  This command will run the fuzzing job as the master process and output results to the 'output_dir' directory."
fi
