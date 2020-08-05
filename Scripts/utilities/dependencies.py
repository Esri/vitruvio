from utilities import paths


dependencies = None


def load_dependencies():
    """
    Loads the binary dependencies.

    :return: None, if there are no binary dependencies.
    :return: The binary dependencies dictionary otherwise.
    """
    try:
        with open(paths.GLOBAL.CONFIG_DEPENDENCIES) as f:
            import json
            return json.load(f)
    except OSError:
        return None


def get_dependencies():
    """
    Loads the dependencies if not loaded.
    Returns the cached version otherwise.

    :return: The dependencies.
    """
    global dependencies
    if dependencies is None:
        dependencies = load_dependencies()
    return dependencies


def get_dependency(dependency):
    """
    Gets the version for the specified dependency.

    :param dependency: The dependency name.

    :return: None, if the dependencies can not be loaded.
    :return: The semantic version of that dependency.
    """
    return get_dependencies()[dependency] if dependency is not None else None
