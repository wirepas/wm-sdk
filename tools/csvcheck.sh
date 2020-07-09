#!/bin/bash

# Script to compare two values in csv files between each other with threshold
# command params
# 1 new csv value
# 2 old csv value
# 3 threshold (like 1.1)

old="$(tail -1 $2)"
new="$(tail -1 $1)"

threshold="$(echo $old \* $3 | bc)"

if (( $(bc <<< "$new > $threshold") ))
then
        echo "error $new > $old"
        exit 1
else
        echo ok
fi
