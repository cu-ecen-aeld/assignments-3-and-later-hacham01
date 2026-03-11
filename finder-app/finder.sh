#!/bin/bash

# Check the number of arguments, it must be 2
if [ $# -ne 2 ]
then
    echo "Error: 2 arguments required"
    exit 1
fi

# use arguments 1 and 2 in variables 
files_dir=$1
search_str=$2

# Check if the path passed in argument 1 is a directory
if [ ! -d "$files_dir" ] 
then 
    echo "Error: $files_dir is not a directory"
    exit 1
fi

# Count the number of files contained in the directory
file_count=$(find "$files_dir" -type f | wc -l)

# Count the number of lines which containe s the string 
matching_lines_count=$(grep -R "$search_str" "$files_dir" | wc -l)

# display the result
echo "The number of files are $file_count and the number of matching lines are $matching_lines_count"

