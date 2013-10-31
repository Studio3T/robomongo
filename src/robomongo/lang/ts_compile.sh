#!/bin/bash

# TRANSLATOR comment, which would be help lupdate with namespaces doesn't works for me... 
# So...

declare -A mapping
mapping[MainWindow]=Robomongo
mapping[SettingsManager]=Robomongo
mapping[EditIndexDialog]=Robomongo
mapping[FindFrame]=Robomongo
mapping[AboutDialog]=Robomongo
mapping[ConnectionsDialog]=Robomongo

for file in `find ./ -name "*.raw.ts"`
do
    right_name=`echo "$file" | sed 's/.raw//g'`
    cp $file $right_name
    for index in ${!mapping[*]}
    do
	sed -i -e "s/<name>$index<\/name>/<name>${mapping[$index]}::$index<\/name>/g" $right_name
    done
    lrelease $right_name
    rm $right_name
done
