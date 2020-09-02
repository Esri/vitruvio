import os
from os import listdir
from os.path import isfile, join
from typing import List


def search_uproject(directory: str):
    uproject_files: List[str] = [f for f in listdir(directory) if
                                 isfile(join(directory, f)) and os.path.splitext(f)[1] == ".uproject"]

    if len(uproject_files) > 0:
        return directory

    parent_dir = os.path.dirname(directory)
    if directory == parent_dir:
        return ""

    return search_uproject(parent_dir)


setup_root = os.getcwd()
uproject_dir = search_uproject(setup_root)

if len(uproject_dir) == 0:
    raise Exception("Could not find uproject file in directory tree of " + setup_root)

print("Found uproject file in directory: " + uproject_dir)


src_encoder = join(setup_root, "UnrealGeometryEncoder")
dest_dir = join(uproject_dir, "Source", "UnrealGeometryEncoder")

if os.path.exists(dest_dir):
    raise Exception(dest_dir + " already exists. Can not symlink encoder library.")

os.symlink(src_encoder, dest_dir)

print("Created symlink from " + src_encoder + " to " + dest_dir)


