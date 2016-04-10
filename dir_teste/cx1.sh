#!/bin/bash
#script - cx1.sh

cnt=1

mkdir -p ficha$cnt

while [ $cnt -le 6 ]
do
  mkdir -p ficha2/prob$cnt
  echo "dir nr.$cnt created!"
  let cnt++
done

