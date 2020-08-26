from pathlib import Path
from utilities import paths
from utilities import log
from utilities import git


log = log.getLogger(__name__)


def load_projects(repository: git.Repository = None, include_plugins=False):
    """
    Loads all Unreal Engine projects for a repository.

    :param repository: If not None, the repository in which to look for projects.
    :param repository: If None, the global repository based on the path file.
    :param include_plugins: If true, loads plugins as well.

    :return: A list of unreal projects in the repository.
    """
    repo_path = repository.PATH_ROOT if repository is not None else paths.GLOBAL.PATH_ROOT

    for project in repo_path.rglob('*.uproject'):
        yield UnrealProject(path=project)

    if include_plugins:
        for plugin in repo_path.rglob('*.uplugin'):
            yield UnrealProject(path=plugin, is_plugin=True)


class UnrealProject:
    def __init__(self, path: Path, is_plugin=False):
        import json

        # Default to 4.25, override if specified differently.
        self.version = '4.25'
        self.name = path.parent.name
        self.plugin = is_plugin
        self.version = None

        if not is_plugin:
            with path.open() as project_file:
                data = json.load(project_file)
                self.version = data['EngineAssociation']

        self.PATH_ROOT = path.parent
        self.PATH_PROJECT = path

        self.PATH_VS_SOLUTION = self.PATH_ROOT.joinpath(self.name).with_suffix('.sln')
        self.PATH_VS_META = self.PATH_ROOT.joinpath('.vs')

        self.PATH_VSC_SOLUTION = self.PATH_ROOT.joinpath(self.name).with_suffix('.code-workspace')
        self.PATH_VSC_META = self.PATH_ROOT.joinpath('.vscode')

        self.PATH_CLION_SOLUTION = self.PATH_ROOT.joinpath(self.name).with_suffix('.project')
        self.PATH_IDEA_META = self.PATH_ROOT.joinpath('.idea')

        self.PATH_CONTENT = self.PATH_ROOT / 'Content'
        self.PATH_INTERMEDIATE = self.PATH_ROOT / 'Intermediate'
        self.PATH_SAVED = self.PATH_ROOT / 'Saved'
        self.PATH_BINARIES = self.PATH_ROOT / 'Binaries'
        self.PATH_CACHE = self.PATH_ROOT / 'DerivedDataCache'
        self.PATH_PLUGINS = self.PATH_ROOT / 'Plugins'
        self.PATH_SOURCE = self.PATH_ROOT / 'Source'
        self.PATH_EXTRAS = self.PATH_ROOT / 'Extras'

    def get_cpp_files(self, include_third_party=False):
        """
        Gets all C++ files in the source directory.

        :param include_third_party: If true, includes files in the third party directory.
        :return: A list of paths to C++ header files in the project.
        """
        from functools import reduce
        all_files = set().union(
            self.PATH_SOURCE.rglob('*.h'),
            self.PATH_SOURCE.rglob('*.hpp'),
            self.PATH_SOURCE.rglob('*.cpp')
        )

        if not include_third_party:
            third_party_files = set().union(
                self.PATH_SOURCE.glob('**/ThirdParty/*.h'),
                self.PATH_SOURCE.glob('**/ThirdParty/*.hpp'),
                self.PATH_SOURCE.glob('**/ThirdParty/*.cpp')
            )

            all_files = all_files - third_party_files

        return all_files

    def cleanup(self, delete_saved=False):
        """
        Cleans up the unreal engine project by deleting all temporary folders and
        the uproject file itself.

        :param delete_saved: If true, deletes the `Saved` folder as well.

        :return: True, if the cleanup was successful.
        :return: False, otherwise.
        """
        from utilities.fs import rmdir

        log.info('{}: Cleaning up.'.format(self.name))

        # Remove all the directories that get regenerated.

        if self.PATH_BINARIES.exists():
            log.info('{}: Removing Binaries...'.format(self.name))
            rmdir(self.PATH_BINARIES)

        if self.PATH_CACHE.exists():
            log.info('{}: Removing Derived Data Cache...'.format(self.name))
            rmdir(self.PATH_CACHE)

        if self.PATH_INTERMEDIATE.exists():
            log.info('{}: Removing Intermediate...'.format(self.name))
            rmdir(self.PATH_INTERMEDIATE)

        if self.PATH_SAVED.exists() and delete_saved:
            log.info('{}: Removing Saved...'.format(self.name))
            rmdir(self.PATH_SAVED)

        # Remove all meta directories and project files.

        log.info('{}: Removing Solution Files'.format(self.name))
        for solution in [
            self.PATH_VS_SOLUTION,
            self.PATH_VSC_SOLUTION,
            self.PATH_CLION_SOLUTION
        ]:
            if solution.exists():
                rmdir(solution)

        for meta in [
            self.PATH_VS_META,
            self.PATH_VSC_META,
            self.PATH_IDEA_META
        ]:
            if meta.exists():
                rmdir(meta)

        log.info('{}: Cleaned up.'.format(self.name))

    def generate(self):
        """
        Regenerates the unreal project solution files using the unreal build tools.

        :note: Needs an environment variable, "UE4PATH" to be set to the engine install folder.
        :note: The engine install folder is the folder with all the 'UE_X.XX' folders.

        :return: True, if the generation succeeded.
        :return: False, if the generation failed.
        """
        from os import getenv
        from pathlib import Path
        from utilities.commands import run

        if self.plugin:
            log.warning('{}: Can not generate project files for a plugin.'.format(self.name))
            return False

        log.info('{}: Generating.'.format(self.name))

        str_ue = getenv('ue4path')
        path_ue= Path(str_ue)

        if path_ue is None or not path_ue .exists():
            log.warning('{}: Could not generate project. "UE4PATH" environment variable missing.'.format(self.name))
            log.warning('{}: Please set the "UE4PATH" environment variable to your Unreal Engine install directory.'.format(self.name))
            log.warning('{}: This is the directory where the "UE_X.XX" folders are located.'.format(self.name))
            return False

        path_engine = path_ue.joinpath('UE_{}'.format(self.version))
        path_ubt = path_engine \
            .joinpath("Engine") \
            .joinpath("Binaries") \
            .joinpath("DotNET") \
            .joinpath("UnrealBuildTool") \
            .with_suffix('.exe')

        if not path_ubt.exists():
            log.warning('{}: UnrealBuildTool could not be found. Please make sure the engine path is correct.'.format(self.name))
            log.warning('{}: Check that "{}" exists and is valid.'.format(self.name, path_ubt))
            return False

        log.info('{}: Generating Project Files...'.format(self.name))

        command_ubt = '{} -ProjectFiles {}'.format(path_ubt.absolute(), self.PATH_PROJECT.absolute())
        process = run(command_ubt, cwd=self.PATH_ROOT)

        if process.is_failure():
            log.warning('{}: Something went wrong while building the project.'.format(self.name))
            log.warning('{}: {}'.format(self.name, process.stderr))
            return False

        log.info('{}: Generated.'.format(self.name))
        return True