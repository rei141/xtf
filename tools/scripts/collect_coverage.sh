#!/bin/bash
out_file=$1
temp_file="temp_accumulate.txt"
touch $temp_file
total=0

# ファイル名をソートして順に処理
for file in $(ls *_all_cov | sort); do
  # 現在のファイルの内容と一時ファイルを結合してsort -> uniq
  cat $file $temp_file | sort | uniq > temp_uniq.txt

  # ユニークな行数をカウント
  lines=$(wc -l < temp_uniq.txt)
  mv temp_uniq.txt $temp_file

  echo "$file: $lines"
  total=$((total + lines))
done

cp $temp_file $out_file
# 一時ファイルを削除
rm $temp_file
