#!/bin/bash
ARGC=$#
i=2  
lupdate_params=""
while [ $i -le $ARGC ]; do 
    lupdate_params="$lupdate_params ${!i}"
    i=$((i+1))
done

if [[ (-z $1) || ($1 == "all")]]
then
    for file in `find ./ -name "*.raw.ts"`
    do
        lupdate $lupdate_params ./.. -ts $file
    done
else 
    lupdate $lupdate_params ./.. -ts ./robomongo_$1.raw.ts
fi
