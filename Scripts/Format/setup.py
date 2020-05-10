#!/usr/bin/env python

import os
import pathlib
import shutil
import subprocess
import sys


def check_is_path_valid(path: pathlib.Path) -> None:
    if not path.exists() or not path.is_dir():
        print("Invalid path: '{}'".format(path_git), file=sys.stderr)
        print("Make sure the path exists and is a valid directory.", file=sys.stderr)
        sys.exit(1)

def check_clang_format() -> None:
    try:
        subprocess.call(["clang-format", "--version"])
    except:
        print("Error: clang-format cannot be found.", file=sys.stderr)
        sys.exit(1)

check_clang_format()

path_file = pathlib.Path(__file__)
path_dir = path_file.parent
path_root = path_dir.parent.parent

# Build the path to the git folder and check if it is valid.
# To be valid, the path must exist and must be a directory.
path_git = path_root.joinpath(".git")
check_is_path_valid(path_git)

# Build the path to the git hooks folder and check if it is valid.
# To be valid, the path must exist and must be a directory.
path_hooks = path_git.joinpath("hooks")
check_is_path_valid(path_hooks)

# Build the path to the git pre-commit hook and check if it already
# exists. If there already is a pre-commit hook, write it to a temporary
# backup file and then write the new one.
path_pre_commit_hook = path_hooks.joinpath("pre-commit")
path_pre_commit_hook_backup = path_hooks.joinpath("pre-commit.backup")
path_pre_commit_hook_install = path_dir.joinpath("pre-commit")
if path_pre_commit_hook.exists():
    print("! pre-commit hook already exists.")
    print("! pre-commit hook backup created at: '{}'".format(path_pre_commit_hook_backup))
    print()
    shutil.move(path_pre_commit_hook, path_pre_commit_hook_backup)

# Install the new pre-commit hook at the target location.
print("> Installing pre-commit hook at '{}'".format(path_pre_commit_hook))
shutil.copyfile(path_pre_commit_hook_install, path_pre_commit_hook)

# Make sure hook is executable
os.chmod(path_pre_commit_hook, 0o755)

print("> Installed pre-commit hook at '{}'".format(path_pre_commit_hook))
print("> Installation complete, you are ready to go.")
sys.exit(0)
