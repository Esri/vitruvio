# Copyright 2020 Esri
#
# Licensed under the Apache License Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

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


