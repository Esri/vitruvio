#!/usr/bin/env python

import argparse
from utilities import log
from utilities import paths
from utilities import clang
from utilities import git


log.prepare()
log = log.getLogger(__name__)
log.info('Setting up git hooks.')


parser = argparse.ArgumentParser(description='Setup repository for development.')
args = parser.parse_args()


# Check clang-format and git versions are valid and abort if not.
clang.ensure_valid()
git.ensure_valid()


# Check git paths and abort if they don't exist.
log.info('Checking required paths.')
paths.ensure_exists(paths.GLOBAL.PATH_GIT)
paths.ensure_exists(paths.GLOBAL.PATH_GIT_HOOKS)

# Build the path to the git pre-commit hook and check if it already
# exists. If there already is a pre-commit hook, write it to a temporary
# backup file and then write the new one.
repository = git.current_repository()
repository.install_hook_utilities()
repository.install_hook('pre-commit')

log.info("Installation complete, you are ready to go.")
