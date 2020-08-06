#!/usr/bin/env python
import argparse
from utilities import git
from utilities import clang
from utilities import copyright
from utilities import log
from utilities import unreal

log.prepare()
log = log.getLogger(__name__)


parser = argparse.ArgumentParser(description='Format all C++ files in the repository.')
parser.add_argument('--plugins', action='store_true', help='Includes plugin folders.')
parser.add_argument('--third-party', action='store_true', help='Includes third party folders.')
args = parser.parse_args()


# Make sure git and clang-format are available in valid versions.
git.ensure_valid()
clang.ensure_valid()

# Get the changed files in the repository.
repository = git.current_repository()
uprojects = unreal.load_projects(repository, include_plugins=args.plugins)

for uproject in uprojects:
    log.info('{}: Unreal Engine Project found!'.format(uproject.name))
    log.info('{}: Formatting C++ source files!'.format(uproject.name))

    for file in uproject.get_cpp_files(include_third_party=args.third_party):
        clang.format_file(file)
        copyright.fix(file)

    log.info('{}: Formatted C++ source files.'.format(uproject.name))
