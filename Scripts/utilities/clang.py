from pathlib import Path
from utilities import paths
from utilities import commands
from utilities import dependencies
from utilities import log


log = log.getLogger(__name__)


def get_version():
    """
    Gets the installed version of clang-format

    :return: None, if clang-format is not found.
    :return: The currently installed version otherwise.
    """
    return commands.get_version('clang-format')


def is_valid():
    """
    Checks if a valid clang-format version is installed.

    :return: True, if clang-format is installed in a valid version.
    :return: False, if clang-format is not installed or not in a valid version.
    """
    required_version = dependencies.get_dependency('clang-format')
    if required_version is None:
        required_version = '0'

    return commands.has_cmd('clang-format', required_version)


def ensure_valid():
    """
    Checks if there is a valid clang format version installed and aborts if not.
    """
    if not is_valid():
        log.error('Invalid clang-format version.')

        installed_version = get_version()
        required_version = dependencies.get_dependency('clang-format')

        log.error('Required clang-format version: {}', required_version)
        log.error('Installed clang-format version: {}', installed_version)

        import sys
        sys.exit(-1)


def format_file(file : Path):
    """
    Runs clang-format on the specified file.

    :note: Will run from the root directory.
    :param file: The absolute path of the file to run clang-format on.

    :return: True, if the file was formatted.
    :return: False, otherwise.
    """
    relative = file.relative_to(paths.GLOBAL.PATH_ROOT)

    if not file.exists() or file.suffix not in ['.cpp', '.h', '.hpp']:
        return False

    else:
        command_format = 'clang-format -style=file -i {}'.format(file)
        process = commands.run(command_format, cwd=paths.GLOBAL.PATH_ROOT)

        if process.is_failure():
            log.warning(process.stderr)
            log.warning('Could not run "clang-format".')
            log.warning('Aborting')
            return False

        log.info('Formatted file "{}"...'.format(relative))
        return True

