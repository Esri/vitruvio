#!/bin/sh

set -e

if ! [ -x "$(command -v clang-format)" ]; then
  echo 'Error: clang-format not installed.' >&2
  exit 1
fi

OUR_DIR="$PWD/`dirname "$0"`"
ROOT_DIR="$OUR_DIR/.."
GIT_DIR="$ROOT_DIR/.git"
HOOK_DIR="$GIT_DIR/hooks"

cd $ROOT_DIR

if [ -d $HOOK_DIR ]; then
    if [ ! -e "$HOOK_DIR/pre-commit" ]; then
        touch "$HOOK_DIR/pre-commit"
        chmod o+x "$HOOK_DIR/pre-commit"
        echo "#!/bin/sh" >> "$HOOK_DIR/pre-commit"
        echo "cd \"\$PWD/\`dirname \"\$0\"\`/../..\"" >> "$HOOK_DIR/pre-commit"
        echo "sh \"Scripts/git_precommit_hook.sh\"" >> "$HOOK_DIR/pre-commit"

        echo "Setup complete!"
    else
        echo "Could not create pre commit hook!"
        echo "Pre commit hook already exists!"
    fi
else
    echo "Could not complete setup!"
    echo "Git hook directory does not exist!"
fi
