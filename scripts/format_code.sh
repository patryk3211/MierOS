#!/bin/bash

CF=clang-format-12

if [ $1 = "" ]; then
    echo "You have to specify atleast 1 input file (Multiple files are separated by semicolons"
    exit
fi

FILES=$(echo $1 | tr ';' ' ')
FILE_COUNT=$(echo $FILES | perl -ne 's/^ *//; s/ *$//; $count = () = $_ =~ / /g; print $count+1')

CURRENT_FILE=0

printf "Formatting %d files\n" $FILE_COUNT
for f in $FILES; do
    CURRENT_FILE=$(expr $CURRENT_FILE + 1)
    printf "[%d/%d] Formatting %s\n" $CURRENT_FILE $FILE_COUNT $f
    $CF $f -i
done
printf "Formatted %d files\n" $FILE_COUNT
