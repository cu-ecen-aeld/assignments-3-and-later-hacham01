#!/bin/bash

# Check the number of arguments, it must be 2
if [ $# -ne 2 ]
then
    echo "Error: 2 arguments required"
    exit 1
fi

# use arguments 1 and 2 in variables 
write_file=$1
write_str=$2

# create directory if id does not exist
mkdir -p "$(dirname "$write_file")"

# write the content into the file (overwrite)
echo "$write_str" > "$write_file"
if [ $? -ne 0 ]
then 
    echo "Error: could not create or write to $write_file"
    exit 1
fi
