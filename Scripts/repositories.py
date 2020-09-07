# /usr/bin/env python

# !/usr/bin/env python

import argparse
import json
from utilities import log
from utilities import paths

# Setup log
log.prepare()
log = log.getLogger(__name__)

# Setup data
config_json = []
config_path = paths.GLOBAL.CONFIG_REPOSITORIES

if config_path.exists():
    with open(config_path, 'r') as config:
        try:
            config_json = json.load(config)
        except json.JSONDecodeError as e:
            log.debug('Could not decode repositories file: {}'.format(e.msg))
            log.debug('Error at line: {}, column: {}'.format(e.lineno, e.colno))


# Actions

def create(arguments):
    """
    Adds a new repository entry to the repositories list.

    :param arguments: The parsed arguments to use.
    """
    global config_json

    name = arguments.name
    remote = arguments.remote
    source = arguments.source
    target = arguments.target

    found = list(filter(lambda repo: repo['Name'] == name, config_json))

    if len(found) > 0:
        log.error('Could not create new repository {}.'.format(name))
        log.error('Repository already exists.')
        log.info('Remove the existing repository first or use a different name.')
        return

    # Create a new entry
    entry = {
        'Name': name,
        'Remote': remote,
        'Source': source,
        'Target': target,
        'Ignore': False
    }

    config_json.append(entry)


def remove(arguments):
    """
    Removes a repository with the specified name.

    :param arguments: 
    """


parser = argparse.ArgumentParser(description='Manage repositories in repositories.json')
subparsers = parser.add_subparsers(title='action', dest='action', required=True)

create_parser = subparsers.add_parser('create')
create_parser.add_argument('name', help="The name of the repository.")
create_parser.add_argument('remote', help="The remote URL of the repository.")
create_parser.add_argument('-s', '--source', nargs="?", default='develop',
                           help="The source branch to use unless overwritten.")
create_parser.add_argument('-t', '--target', nargs="?", default='devops',
                           help="The target branch to use unless overwritten.")
create_parser.set_defaults(func=create)

args = parser.parse_args()
if 'func' in args:
    args.func(args)

with open(config_path, 'w') as config:
    json.dump(config_json, config, indent=True)
