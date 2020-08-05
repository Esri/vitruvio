def rmdir(path):
    """
    Removes a directory, including readonly files.

    :param path: The path to remove.
    """
    from shutil import rmtree
    from os import chmod, remove
    import stat

    def onerror(action, name, exception):
        chmod(name, stat.S_IWRITE)
        remove(name)

    if path.exists():
        rmtree(path, onerror=onerror)


def safe_copy_tree(src, dst):
    """
    Safely copies one tree to another location.

    Deletes the destination if it already exists.
    Careful, this operation is not transactional.

    :param src: The source path to copy.
    :param dst: The target path to copy  to.
    """
    from shutil import copytree
    from utilities.paths import GLOBAL

    def ignore(current, files):
        return [
            str(GLOBAL.PATH_TEMPLATES.relative_to(GLOBAL.PATH_SCRIPTS)),
            'distribute.py',
            'repositories.json',
            'README.md',
            '__pycache__',
            '.idea', '.git', '.vs', '.vscode'
        ]

    rmdir(dst)
    copytree(src, dst, ignore=ignore)
