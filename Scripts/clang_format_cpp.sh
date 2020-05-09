#!/bin/sh

set -e

if ! [ -x "$(command -v clang-format)" ]; then
  echo 'Error: clang-format not installed.' >&2
  exit 1
fi

TARGET_FOLDER="$PWD/$1"

echo "Formatting: $TARGET_FOLDER"

if [ -d "$TARGET_FOLDER" ]; then
    TARGET_FILES=`find $TARGET_FOLDER -name "*.cpp" -o -name "*.h"`
    for FILE in $TARGET_FILES; do

        # Make sure we only try to format cpp and h files.
        # Otherwise everything gets properly screwed.
        if [ -e $FILE ]; then
            echo "Formatting `basename -- $FILE`"
            clang-format -style=file -i $FILE 
        fi
    done
fi
