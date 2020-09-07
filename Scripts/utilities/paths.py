from pathlib import Path
from utilities import log


log = log.getLogger(__name__)


def ensure_exists(path: Path):
    """
    Checks if the specified path exists and aborts if not.

    :note: Aborts with a return code of -1.
    :param path: The path to check.
    """
    if not path.exists():
        log.error('Path does not exist: {}', path)
        log.error('Aborting')

        import sys
        sys.exit(-1)


def filter_cpp_files(paths: [Path]):
    """
    Filters a list of paths and returns only C++ files.

    :note: Supports .h, .cpp and .hpp files.
    :return: A list of C++ file paths.
    """

    # Quick helper function to check if a file is a path.
    def is_cpp_file(target: Path):
        return target.is_file() and target.suffix in ['.cpp', '.h', '.hpp']

    # Return only files that are cpp files.
    return filter(is_cpp_file, paths)


class Paths:
    def __init__(self, root):
        # -----------------------------------------------------------------------------
        # Important Directory Paths
        #

        self.PATH_ROOT: Path = root
        self.PATH_SCRIPTS: Path = self.PATH_ROOT.joinpath('Scripts')
        self.PATH_GIT: Path = self.PATH_ROOT.joinpath('.git')
        self.PATH_GIT_HOOKS: Path = self.PATH_GIT.joinpath('hooks')
        self.PATH_GIT_UTILITIES: Path = self.PATH_GIT_HOOKS.joinpath('utilities')
        self.PATH_CHECKOUTS: Path = self.PATH_ROOT.joinpath('Checkouts')

        # -----------------------------------------------------------------------------
        # Script Directory Paths
        #

        self.PATH_UTILITIES: Path = self.PATH_SCRIPTS.joinpath('utilities')
        self.PATH_HOOKS: Path = self.PATH_SCRIPTS.joinpath('hooks')
        self.PATH_CONFIG: Path = self.PATH_SCRIPTS.joinpath('config')
        self.PATH_TEMPLATES: Path = self.PATH_SCRIPTS.joinpath('templates')

        # -----------------------------------------------------------------------------
        # Configuration File Paths
        #

        self.CONFIG_REPOSITORIES: Path = self.PATH_CONFIG.joinpath('repositories.json')
        self.CONFIG_DEPENDENCIES: Path = self.PATH_CONFIG.joinpath('dependencies.json')

        # -----------------------------------------------------------------------------
        # Template File Paths
        #

        self.TEMPLATE_GITIGNORE: Path = self.PATH_TEMPLATES.joinpath('.gitignore.template')
        self.TEMPLATE_GITATTRIBUTES: Path = self.PATH_TEMPLATES.joinpath('.gitattributes.template')
        self.TEMPLATE_CLANGFORMAT: Path = self.PATH_TEMPLATES.joinpath('.clang-format.template')
        self.TEMPLATE_README: Path = self.PATH_TEMPLATES.joinpath('README.md')

        # -----------------------------------------------------------------------------
        # Important File Paths
        #

        self.GITIGNORE: Path = self.PATH_ROOT.joinpath('.gitignore')
        self.GITATTRIBUTES: Path = self.PATH_ROOT.joinpath('.gitattributes')
        self.CLANGFORMAT: Path = self.PATH_ROOT.joinpath('.clang-format')
        self.README: Path = self.PATH_SCRIPTS.joinpath('README.md')


root = Path(__file__).parent.parent.parent
root = root.parent if root.name == '.git' else root
GLOBAL: Paths = Paths(root)
