#!/bin/sh

set -e 
cd "`dirname "$0"`/.."

if ! [ -x "$(command -v clang-format)" ]; then
  echo 'Warning: clang-format not installed, skipping.' >&2
  exit 0
fi

D_GIT_HOOKS=".git/hooks" 
F_GIT_HOOKS_COMMIT="$D_GIT_HOOKS/.commit"

if [ -d "$D_GIT_HOOKS" ]; then
    # Setup a commit file that contains the paths to all files
    # in the current commit. This file is later reused to run
    # commands only on files that have been changed.
    touch $F_GIT_HOOKS_COMMIT
    git diff --name-only --cached >> $F_GIT_HOOKS_COMMIT

    CHANGED_FILES=`cat $F_GIT_HOOKS_COMMIT`
    for FILE in $CHANGED_FILES; do
        # Make sure we only try to format cpp and h files.
        # Otherwise everything gets properly screwed.
        if [ ${FILE: -4} == ".cpp" ] || [ ${FILE: -2} == ".h" ]; then
            if test -f $FILE; then 
                echo "Formatting `basename -- $FILE`"

                clang-format -style=file -i $FILE 
                git add $FILE 
            fi
        fi
    done
    
    rm $F_GIT_HOOKS_COMMIT 
fi
