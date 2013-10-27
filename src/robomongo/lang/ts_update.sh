#!/bin/bash
if [[ -z $1 ]] 
then
    lupdate ./.. -ts ./robomongo_dummy.raw.ts
else 
    lupdate ./.. -ts ./robomongo_$1.raw.ts
fi
