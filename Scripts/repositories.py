#!/usr/bin/env python

import argparse
import json
from utilities import log
from utilities import paths


class Entry:
    def __init__(self, name=None, remote=None, source=None, target=None, ignore=True):
        self.name = name
        self.remote = remote
        self.source = source
        self.target = target
        self.ignore = ignore


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
            config_json = list(map(lambda entry: Entry(
                name=entry['Name'],
                remote=entry['Remote'],
                source=entry['Source'],
                target=entry['Target'],
                ignore=entry['Ignore']
            ), config_json))
        except json.JSONDecodeError as e:
            log.debug('Could not decode repositories file: {}'.format(e.msg))
            log.debug('Error at line: {}, column: {}'.format(e.lineno, e.colno))


# Actions

def register(arguments):
    """
    Adds a new repository entry to the repositories list.

    :param arguments: The parsed arguments to use.
    """
    global config_json

    name = arguments.name
    remote = arguments.remote
    source = arguments.source
    target = arguments.target

    if name is None:
        from re import findall
        regex = r'[/\\](?P<name>[^\\/]+)\.git'
        results = findall(regex, remote)
        name = results[0] if len(results) > 0 else 'Unknown'

    found = list(filter(lambda repo: repo.name == name, config_json))

    if len(found) > 0:
        log.error('Could not create new repository {}.'.format(name))
        log.error('Repository already exists.')
        log.info('Remove the existing repository first or use a different name.')
        return

    # Create a new entry
    entry = Entry(
        name=name,
        remote=remote,
        source=source,
        target=target,
        ignore=False
    )

    config_json.append(entry)


def remove(arguments):
    """
    Removes a repository with the specified name.

    :param arguments: The arguments parsed.
    """
    for name in arguments.names:
        found = list(filter(lambda entry: entry[1].name == name, enumerate(config_json)))

        if len(found) == 0:
            log.error('Could not remove repository {}.'.format(name))
            log.error('Repository does not exist.')
            continue

        index = found[0][0]
        del config_json[index]


def disable(arguments):
    """
    Disables repositories with the specified names.

    :param arguments: The arguments parsed.
    """
    for name in arguments.names:
        found = list(filter(lambda entry: entry.name == name, config_json))

        if len(found) == 0:
            log.error('Could not disable repository {}.'.format(name))
            log.error('Repository does not exist.')
            continue

        repository = found[0]
        if repository.ignore:
            log.warning('Trying to disable repository {}.'.format(repository.name))
            log.warning('Repository is already disabled.')
        else:
            found[0].ignore = True


def enable(arguments):
    """
    Enables repositories with the specified names.

    :param arguments: The arguments parsed.
    """
    for name in arguments.names:
        found = list(filter(lambda entry: entry.name == name, config_json))

        if len(found) == 0:
            log.error('Could not disable repository {}.'.format(name))
            log.error('Repository does not exist.')
            continue

        repository = found[0]
        if not repository.ignore:
            log.warning('Trying to enable repository {}.'.format(repository.name))
            log.warning('Repository is already enabled.')
        else:
            found[0].ignore = False


def profile(arguments):
    """
    Sets the source and target of profiles with the specified names depending on the mode.

    :direct:    Source 'develop', Target 'develop'.
    :append:    Source 'devops', Target 'devops'.
    :pull:      Source 'develop', Target 'devops'.

    :param arguments: The arguments parsed.
    """
    for name in arguments.names:
        found = list(filter(lambda entry: entry.name == name, config_json))

        if len(found) == 0:
            log.error('Could not set profile for repository {}'.format(name))
            log.error('Repository does not exist.')
            continue

        profiles = {
            'direct' : ('develop', 'develop'),
            'append' : ('devops', 'devops'),
            'pull' : ('develop', 'devops')
        }

        repository = found[0]
        repository.source, repository.target = profiles[arguments.profile]


parser = argparse.ArgumentParser(description='Manage repositories in repositories.json')
subparsers = parser.add_subparsers(title='action', dest='action')

# Allow registering new repositories.
parse_register = subparsers.add_parser('register', description='Register a new repository.')
parse_register.add_argument('remote', help="The remote URL of the repository.")
parse_register.add_argument('-n', '--name', nargs="?", help="The name of the repository.")
parse_register.add_argument('-s', '--source', nargs="?", default='develop',
                            help="The source branch to use unless overwritten.")
parse_register.add_argument('-t', '--target', nargs="?", default='devops',
                            help="The target branch to use unless overwritten.")
parse_register.set_defaults(func=register)

# Allow completely removing repositories.
parse_remove = subparsers.add_parser('remove', description='Remove repositories.')
parse_remove.add_argument('names', nargs='+', help='The names of the repositories.')
parse_remove.set_defaults(func=remove)

# Allow disabling repositories.
parse_disable = subparsers.add_parser('disable', description='Disable repositories.')
parse_disable.add_argument('names', nargs='+', help='The names of the repositories.')
parse_disable.set_defaults(func=disable)

# Allow enabling repositories.
parse_enable = subparsers.add_parser('enable', description='Enable repositories.')
parse_enable.add_argument('names', nargs='+', help='The names of the repositories.')
parse_enable.set_defaults(func=enable)

# Allow changing source
parse_profile = subparsers.add_parser('profile', description='Sets a profile for repositories.')
parse_profile.add_argument('profile', choices=['direct', 'append', 'pull'], help='The profile to use.')
parse_profile.add_argument('names', nargs='+', help='The names of the repositories.')
parse_profile.set_defaults(func=profile)

args = parser.parse_args()

if 'action' in args and 'func' in args:
    args.func(args)
else:
    import functools

    names = map(lambda entry: entry.name, config_json)
    name_padding = len(functools.reduce(lambda current, name: current if len(current) > len(name) else name, names))

    remotes = map(lambda entry: entry.remote, config_json)
    remote_padding = len(functools.reduce(lambda current, remote: current if len(current) > len(remote) else remote, remotes))

    sources = map(lambda entry: entry.source, config_json)
    source_padding = len(functools.reduce(lambda current, source: current if len(current) > len(source) else source, sources))

    targets = map(lambda entry: entry.target, config_json)
    target_padding = len(functools.reduce(lambda current, target: current if len(current) > len(target) else target, targets))

    for repository in config_json:
        print('{} {} : {} : {} -> {}'.format(
            'X' if repository.ignore else 'E',
            repository.name.rjust(name_padding),
            repository.remote.rjust(remote_padding),
            repository.source.rjust(source_padding),
            repository.target.rjust(target_padding)))

with open(config_path, 'w') as config:
    config_json = list(map(lambda entry: {
        'Name': entry.name,
        'Remote': entry.remote,
        'Source': entry.source,
        'Target': entry.target,
        'Ignore': entry.ignore
    }, config_json))
    json.dump(config_json, config, indent=True)
