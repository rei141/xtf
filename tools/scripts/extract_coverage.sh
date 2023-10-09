#!/bin/bash

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <coverage_file_path_or_directory> <directory_path_or_filename>"
    exit 1
fi

input_path=$1
secondary_path=$2
echo $input_path
# パスがディレクトリの場合
if [ -d "$input_path" ]; then
    directory_path=$input_path
    coverage_file_path=$secondary_path
    rm /tmp/all_cov -f
    # ディレクトリ内のすべてのファイルを反復処理
    find "$directory_path" -type f -name '*gcov*' | while read -r filename; do
        echo "Processing file: $filename"
        awk -v file="$coverage_file_path" '
        BEGIN { print_data=0 }
        $0 ~ "  -:    0:Source:" && print_data { exit }
        $0 ~ "Source:"file { print_data=1 }
        print_data { print }
        ' "$filename" | grep -v "\-\:" | grep -v "#####" | cut -d ":" -f 2 | sed -e "s/ //g" >> /tmp/all_cov
    done
    directory=$(basename "$directory_path")
    cat /tmp/all_cov | sort | uniq > $directory"_all_cov"
    cat $directory"_all_cov" | wc
# パスがファイルの場合
elif [ -f "$input_path" ]; then
    filename=$input_path
    coverage_file_path=$secondary_path
    awk -v file="$coverage_file_path" '
    BEGIN { print_data=0 }
    $0 ~ "  -:    0:Source:" && print_data { exit }
    $0 ~ "Source:"file { print_data=1 }
    print_data { print }
    ' "$filename"

else
    echo "Error: The provided path is neither a directory nor a file."
    exit 1
fi