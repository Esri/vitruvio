def has_cmd(cmd, version=None):
    """
    Checks if the local machine has the specified command in its' path.
    Note that the version is expected to be semantic.

    :param cmd: The command to check for.
    :param version: The minimum version to check for.

    :return: True, if git is available.
    :return: False, if git is unavailable.
    """
    from shutil import which

    if which(cmd) is None:
        return False

    if version is not None:
        reported = get_version(cmd)
        return reported is not None and reported >= version

    return True


def get_version(cmd):
    """
    Gets the version of the specified command.

    :param cmd: The command to check.

    :return: None, if no version was found.
    :return: A version string, if a version was found.
    """
    from re import search

    command = run('{} --version'.format(cmd))
    if command.is_failure():
        return None

    for line in command.stdout.splitlines():
        # Find line with version tag
        if search('[Vv]ersion', line) is None:
            continue

        # Check if version tag exists
        matched = search(r'\d+\.\d+\.\d+', line)
        if matched is None:
            continue

        # Check if version tag is high enough (lex)
        return matched.group()

    return None


def run(cmd, cwd=None, env=None):
    """
    Runs the specified command in a subprocess synchronously.

    :param cmd: The command to run.
    :param cwd: The working directory to run the command in.
    :param env: The environment the command is run in.

    :return: A command result wrapper.
    """
    return Command(cmd, cwd, env)


class Command:
    def __init__(self, cmd, cwd=None, env=None):
        """
        Creates a new command container.

        Note that this immediately executes the command synchronously and writes the
        return values to the corresponding members.

        :param cmd: The command to execute.
        :param cwd: The working directory to run the command in.
        :param env: The environment the command is run in.
        """
        import subprocess as p
        process = p.Popen(cmd, stdout=p.PIPE, stderr=p.PIPE, shell=True, cwd=cwd, env=env)
        stdout, stderr = process.communicate()

        self.stdout = stdout.decode('utf8')
        self.stderr = stderr.decode('utf8')

        self.return_code = process.returncode
        process.kill()

    def is_success(self):
        return self.return_code == 0

    def is_failure(self):
        return self.return_code != 0
