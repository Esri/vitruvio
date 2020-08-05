#!/usr/bin/env python
from utilities import git
from utilities import clang
from utilities import copyright
from utilities import log
from utilities import unreal

log.prepare()
log = log.getLogger(__name__)

# Make sure git and clang-format are available in valid versions.
git.ensure_valid()
clang.ensure_valid()

# Get the changed files in the repository.
repository = git.current_repository()
uprojects = unreal.load_projects(repository)

for uproject in uprojects:
    log.info('{}: Unreal Engine Project found!'.format(uproject.name))
    log.info('{}: Formatting C++ source files!'.format(uproject.name))

    for file in uproject.get_cpp_files():
        clang.format_file(file)
        copyright.fix(file)

    log.info('{}: Formatted C++ source files.')
