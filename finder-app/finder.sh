#!/bin/sh

if [ $# != 2 ] ; then
    echo Usage: $0 filesdir searchstr
    exit 1
fi

filesdir="$1"
searchstr="$2" 

if [ ! -d "$filesdir" ] ; then
    echo "$filesdir" is not a directory
    exit 1
fi

# Print the message:
#  "The number of files are X and the number of matching lines are Y"

number_of_files=`find "$filesdir" -type f | wc -l`
number_of_matching_lines=`grep -r "$searchstr" "$filesdir" | wc -l`

echo "The number of files are $number_of_files and the number of matching lines are $number_of_matching_lines"


