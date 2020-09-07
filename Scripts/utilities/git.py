from utilities import paths
from utilities import commands
from utilities import dependencies
from utilities import fs
from utilities import log
from pathlib import Path

log = log.getLogger(__name__)


def get_version():
    """
    Gets the installed version of git.

    :return: None, if git is not found.
    :return: The currently installed version otherwise.
    """
    return commands.get_version('git')


def is_valid():
    """
    Checks if there is a valid git version installed.

    :return: True, if git is installed in a valid version.
    :return: False, if git is not installed or not in a valid version.
    """
    required_version = dependencies.get_dependency('git')
    if required_version is None:
        required_version = '0'

    return commands.has_cmd('git', required_version)


def ensure_valid():
    """
    Checks if there is a valid git version installed and aborts if not.
    """
    if not is_valid():
        log.error('Invalid git version.')

        installed_version = get_version()
        required_version = dependencies.get_dependency('git')

        log.error('Required git version: {}', required_version)
        log.error('Installed git version: {}', installed_version)

        import sys
        sys.exit(-1)


def load_repositories():
    """
    Loads all repositories from the configuration file.

    :return: None, if there is no configuration file.
    :return: An array of repository dictionaries otherwise.
    """
    from json import load

    if not paths.GLOBAL.CONFIG_REPOSITORIES.exists():
        log.warning('No repository configuration file found at "{}"'.format(paths.GLOBAL.CONFIG_REPOSITORIES))
        log.warning('Please create a "repositories.json" file at the specified location.')
        log.warning('Please consult the scripts readme file for examples.')
        return None

    with paths.GLOBAL.CONFIG_REPOSITORIES.open() as f:
        repositories = load(f)
        output = []
        for repository in repositories:
            if repository['Ignore']:
                continue

            name = repository['Name']
            remote = repository['Remote']
            source = repository['Source']
            target = repository['Target']
            directory = paths.GLOBAL.PATH_CHECKOUTS.joinpath(name)

            output.append(Repository(
                directory,
                name,
                remote,
                source,
                target,
                cleanup=True
            ))

        return output


def current_repository():
    """
    Gets the current git repository.

    The current git repository is the repository the paths python file is
    located in. It represents the repository from which the script is
    being executed.
    """
    return Repository(root=paths.GLOBAL.PATH_ROOT)


class Repository(paths.Paths):
    def __init__(self, root: Path, name=None, remote=None, source=None, target=None, cleanup=False):
        super().__init__(root)

        self.name = name
        self.remote = remote
        self.source = source
        self.target = target

        self.cleanup = cleanup

        if self.name is None:
            self.name = self.PATH_ROOT.name
        if self.source is None:
            self.source = 'develop'
        if self.target is None:
            self.target = 'develop'

    def __del__(self):
        if self.cleanup:
            fs.rmdir(self.PATH_ROOT)

    def local_branches(self):
        """
        Gets all local branches of the repository.

        :return: A list of all local branches if successful.
        :return: An empty list if unsuccessful.
        """
        command_branch = 'git branch'
        process = commands.run(command_branch, cwd=self.PATH_ROOT)

        if process.is_failure():
            return []

        branches = process.stdout.splitlines()
        return branches

    def all_branches(self):
        """
        Gets all branches of the repository.

        :return: A list of all branches if successful.
        :return: An empty list if unsuccessful.
        """
        command_branch = 'git branch -la'
        process = commands.run(command_branch, cwd=self.PATH_ROOT)

        if process.is_failure():
            return []

        branches = process.stdout.splitlines()
        branches = [branch.strip() for branch in branches]
        return branches

    def changed_files(self):
        """
        Returns the changed files compare to the last commit.

        :return: A list of changed files.
        :return: An empty list on failure.
        """
        command_diff = 'git diff --name-only HEAD~1'
        process = commands.run(command_diff, cwd=self.PATH_ROOT)

        if process.is_failure():
            return []

        lines = process.stdout.splitlines()
        names = [line.strip() for line in lines]
        files = [paths.GLOBAL.PATH_ROOT.joinpath(name) for name in names]

        return files

    def staged_files(self):
        """
        Returns the currently staged files.

        :return: A list of staged files.
        :return: An empty list on failure.
        """
        command_diff = 'git diff --name-only --staged'
        process = commands.run(command_diff, cwd=self.PATH_ROOT)

        if process.is_failure():
            return []

        lines = process.stdout.splitlines()
        names = [line.strip() for line in lines]
        files = [paths.GLOBAL.PATH_ROOT.joinpath(name) for name in names]

        return files

    def clone(self, slim=True):
        """
        Clones the repository.

        :param slim: If true, does not clone LFS entries.

        :return: False, if cloning failed.
        :return: True, if cloning succeeded.
        """
        from os import environ

        if not is_valid() or self.remote is None:
            return False

        if self.PATH_ROOT.exists():
            fs.rmdir(self.PATH_ROOT)

        log.info('{}: Cloning repository {}'.format(self.name, self.name))
        log.info('{}: Cloning from {}'.format(self.name, self.remote))
        log.info('{}: Cloning branch {}'.format(self.name, self.source))

        if not paths.GLOBAL.PATH_CHECKOUTS.exists():
            log.warning('{}: Can not clone repository. Checkouts folder does not exist.'.format(self.name, self.name))
            log.warning(
                '{}: Creating new checkouts folder at: "{}"'.format(self.name, str(paths.GLOBAL.PATH_CHECKOUTS.parent)))
            paths.GLOBAL.PATH_CHECKOUTS.mkdir(parents=True)

        environment = dict(environ, **{"GIT_LFS_SKIP_SMUDGE": "1"}) if slim else None
        command = 'git clone --branch {} {} {}'.format(self.source, self.remote, self.name)
        process = commands.run(command, cwd=paths.GLOBAL.PATH_CHECKOUTS, env=environment)

        if process.is_failure():
            log.warning(process.stderr)
            log.warning('{}: Cloning repository {} failed!'.format(self.name, self.name))
            return False

        log.info('{}: Cloning repository {} done!'.format(self.name, self.name))
        log.info('{}: Checking out new target branch {}'.format(self.name, self.target))

        if self.target != self.source:
            command = 'git checkout -b {}'.format(self.target)
            process = commands.run(command, cwd=self.PATH_ROOT)

            if process.is_failure():
                log.warning(process.stderr)
                log.warning('{}: Checking out target branch {} failed!'.format(self.name, self.target))
                return False

        return True

    def add_all(self):
        """
        Stages all changed or new files.

        :return: False, if git fails on any command.
        :return: True, otherwise.
        """
        command_add = 'git add --all'
        process = commands.run(command_add, cwd=self.PATH_ROOT)

        if process.is_failure():
            log.warning(process.stderr)
            log.warning('{}: Could not run "git add".'.format(self.name, self.name))
            log.warning('{}: Aborting.'.format(self.name, self.name))
            return False

        return True

    def add_u(self):
        """
        Stages all updated files.

        :return: False, if git fails on any command.
        :return: True, otherwise.
        """
        command_add = 'git add -u'
        process = commands.run(command_add, cwd=self.PATH_ROOT)

        if process.is_failure():
            log.warning(process.stderr)
            log.warning('{}: Could not run "git add".'.format(self.name, self.name))
            log.warning('{}: Aborting.'.format(self.name, self.name))
            return False

        return True

    def add(self, path):
        """
        Stages all updated files.

        :param path: The file to add.

        :return: False, if git fails on any command.
        :return: True, otherwise.
        """
        command_add = 'git add {}'.format(path)
        process = commands.run(command_add, cwd=self.PATH_ROOT)

        if process.is_failure():
            log.warning(process.stderr)
            log.warning('{}: Could not run "git add".'.format(self.name, self.name))
            log.warning('{}: Aborting.'.format(self.name, self.name))
            return False

        return True

    def commit(self, msg):
        """
        Adds all changes to the repository and creates a commit.

        :param msg: The commit message to use.

        :return: False, if git has not been installed.
        :return: False, if git fails on any command.
        :return: True, if the commit succeeds.
        """
        if not is_valid():
            return False

        log.info('{}: Committing on repository {}.'.format(self.name, self.name))

        command_commit = 'git commit -m "{} (DEVOPS)"'.format(msg)
        process = commands.run(command_commit, cwd=self.PATH_ROOT)

        if process.is_failure():
            log.warning(process.stderr)
            log.warning('{}: Could not run "git commit".'.format(self.name, self.name))
            log.warning('{}: Aborting.'.format(self.name, self.name))
            return False

        for line in self.changed_files():
            log.info('{}: Updated file {}.'.format(self.name, line))

        return True

    def can_push(self):
        """
        Performs a safety check in case you are trying to push to develop or master.

        :return: False, if the push needs to be aborted.
        :return: True, if the push can go ahead.
        """
        import re

        regex_yes = re.compile('[Yy]es')
        regex_no = re.compile('[Nn]o')

        match_yes = False
        match_no = False

        if self.target == 'develop' or self.target == 'master':
            while True:
                response = input('You are about to push to {}. Confirm? (yes/no): '.format(self.target))
                print()

                match_yes = regex_yes.match(response) is not None
                match_no = regex_no.match(response) is not None

                if match_yes or match_no:
                    break

            if match_no:
                log.warning('{}: Aborting push to {}.'.format(self.name, self.target))
                return False

        elif self.target == 'devops' and self.target != self.source \
                and 'remotes/origin/{}'.format(self.target) in self.all_branches():
            while True:
                response = input('Remote already has a "{}" branch. Delete? (yes/no): '.format(self.target))
                print()

                match_yes = regex_yes.match(response) is not None
                match_no = regex_no.match(response) is not None

                if match_yes or match_no:
                    break

            if match_yes:
                log.info('{}: Deleting remote "{}" branch.'.format(self.name, self.target))

                command_delete = 'git push origin :{}'.format(self.target)
                process = commands.run(command_delete, cwd=self.PATH_ROOT)

                if process.is_failure():
                    log.warning(process.stderr)
                    log.warning('{}: Could not delete remote "{}" branch.'.format(self.name, self.target))
                    return False

                log.info('{}: Deleted remote "{}" branch.'.format(self.name, self.target))

            if match_no:
                log.warning('{}: Aborting push to devops.'.format(self.name, self.name))
                return False

        return True

    def push(self):
        """
        Pushes all commits to the remote repository.

        :return: False, if the push failed.
        :return: True, if the push succeeded.
        """
        if not is_valid() or self.remote is None or not self.can_push():
            return False

        log.info('{}: Pushing on repository {}.'.format(self.name, self.name))
        log.info('{}: Pushing to branch {}.'.format(self.name, self.target))

        command_push = 'git push origin {}'.format(self.target)
        process = commands.run(command_push, cwd=self.PATH_ROOT)

        if process.is_failure():
            log.warning(process.stderr)
            log.warning('{}: Could not run "git push".'.format(self.name, self.name))
            return False

        return True

    def install_gitignore(self):
        """
        Installs the global gitignore file to the repository.
        Does not add the file or create any commits.
        """
        from shutil import copy

        src = paths.GLOBAL.TEMPLATE_GITIGNORE
        dst = self.GITIGNORE

        if src == dst or not src.exists():
            return

        src = str(src)
        dst = str(dst)

        log.info('{}: Installing gitignore file.'.format(self.name, self.name))
        log.debug('{}: Gitignore source: "{}"'.format(self.name, src))
        log.debug('{}: Gitignore target: "{}"'.format(self.name, dst))

        copy(src, dst)

    def install_gitattributes(self):
        """
        Installs the global gitattributes file to the repository.
        Does not add the file or create any commits.
        """
        from shutil import copy

        src = paths.GLOBAL.TEMPLATE_GITATTRIBUTES
        dst = self.GITATTRIBUTES

        if src == dst or not src.exists():
            return

        src = str(src)
        dst = str(dst)

        log.info('{}: Installing gitattribute file.'.format(self.name, self.name))
        log.debug('{}: Gitattribute source: "{}"'.format(self.name, src))
        log.debug('{}: Gitattribute target: "{}"'.format(self.name, dst))

        copy(src, dst)

    def install_clang_format(self):
        """
        Installs the global clang format file to the repository.
        Does not add the file or create any commits.
        """
        from shutil import copy

        src = paths.GLOBAL.TEMPLATE_CLANGFORMAT
        dst = self.CLANGFORMAT

        if src == dst or not src.exists():
            return

        src = str(src)
        dst = str(dst)

        log.info('{}: Installing clang format file.'.format(self.name, self.name))
        log.debug('{}: Clang format source: "{}"'.format(self.name, src))
        log.debug('{}: Clang format target: "{}"'.format(self.name, dst))

        copy(src, dst)

    def install_readme(self):
        """
        Installs the global readme file to the repository.
        Does not add the file or create any commits.
        """
        from shutil import copy

        src = paths.GLOBAL.TEMPLATE_README
        dst = self.README

        if src == dst or not src.exists():
            return

        src = str(src)
        dst = str(dst)

        log.info('{}: Installing readme file.'.format(self.name, self.name))
        log.debug('{}: Readme source: "{}"'.format(self.name, src))
        log.debug('{}: Readme target: "{}"'.format(self.name, dst))

        copy(src, dst)

    def install_scripts(self):
        """
        Installs the global scripts folder to the repository.
        Does not add the files or create any commits.
        """
        src = paths.GLOBAL.PATH_SCRIPTS
        dst = self.PATH_SCRIPTS

        if src == dst or not src.exists():
            return

        log.info('{}: Installing scripts to repository directory.'.format(self.name, self.name))
        log.debug('{}: Scripts source path: "{}"'.format(self.name, src))
        log.debug('{}: Scripts target path: "{}"'.format(self.name, dst))

        fs.safe_copy_tree(src, dst)
        self.install_readme()

    def safe_install_hooks(self, *hooks, symbolic=True):
        """
        Installs all hooks provided to the function in a safe manner.

        :note:
            Symbolic links might not be available due to access right issues.
            This function will check if the symlink creation failed and, if so,
            will also check if you are running with elevated rights and report
            if not.

        :note:
            Currently, does not cleanup already installed hooks if the process
            fails on a later hook due to invalid access rights. This might leave
            git in an erroneous state.

        :returns: False, if the hooks could not be installed.
        :returns: True, if the hooks could be installed.

        :throws: Any other unknown errors that might occur.
        """
        # TODO (MM): Add proper cleanup.

        try:
            for hook in hooks:
                self.install_hook(hook, symbolic=symbolic)

            if len(hooks) > 0:
                self.install_hook_utilities(symbolic=symbolic)
        except OSError as os_error:
            log.error('Could not install git hooks. An error occurred.')
            log.info('Checking whether it could be due to missing admin rights.')

            from utilities.admin import is_admin

            if not is_admin():
                log.warning('Please try to run again with admin rights.')
                return False
            else:
                log.warning('Admin rights available. Unknown reason for failure.')
                log.warning('Please report this error to your devops admin.')
                raise os_error

        return True

    def install_hook_utilities(self, symbolic=True):
        """
        Installs the latest script utilities to the git hooks folder.

        :param symbolic: If true, installs a symbolic link.
        """
        from os import symlink

        src = self.PATH_UTILITIES
        dst = self.PATH_GIT_UTILITIES

        if src == dst or not src.exists():
            return

        str_src = str(src)
        str_dst = str(dst)

        log.info('{}: Installing utilities to hooks directory.'.format(self.name, self.name))
        log.debug('{}: Utilities source path: "{}"'.format(self.name, src))
        log.debug('{}: Utilities target path: "{}"'.format(self.name, dst))

        if symbolic:
            fs.rmdir(dst)
            symlink(str_src, str_dst, target_is_directory=True)
        else:
            fs.safe_copy_tree(src, dst)

    def install_hook(self, name, symbolic=True):
        """
        Installs the git hook with the specified name to the repository.
        Creates a backup of an existing hook, if a hook exists.

        :param name: The name of the hook to install.
        :param symbolic: If true, installs a symbolic link.

        :return: True, if the hook could be installed.
        :return: False, otherwise.
        """
        from shutil import copy, move
        from os import chmod, symlink

        src = self.PATH_HOOKS.joinpath('hook.{}'.format(name))
        dst = self.PATH_GIT_HOOKS.joinpath(name)

        if src == dst or not src.exists():
            return

        log.info('{}: Installing {}-hook.'.format(self.name, name))

        str_src = str(src)
        str_dst = str(dst)

        if dst.exists():
            path_backup = dst.with_suffix('.backup')
            str_backup = str(path_backup)

            log.warning('{}: {}-hook already exists at target.'.format(self.name, name))
            log.info('{}: Creating backup of {}-hook.'.format(self.name, name))

            if path_backup.exists():
                path_backup.unlink()
            move(str_dst, str_backup)

            path_backup = path_backup.relative_to(paths.GLOBAL.PATH_ROOT)
            log.info('{}: Created backup of {}-hook at {}.'.format(self.name, name, path_backup))

        log.debug('{}: Hook source path: "{}"'.format(self.name, src))
        log.debug('{}: Hook target path: "{}"'.format(self.name, dst))

        if symbolic:
            symlink(str_src, str_dst)
            chmod(str_dst, 0o755)
        else:
            copy(str_src, str_dst)
            chmod(str_dst, 0o755)

        log.info('{}: {}-hook installed.'.format(self.name, name))
