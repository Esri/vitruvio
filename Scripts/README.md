# Scripts

This folder contains various utility scripts that are used to facilitate setting devops related tasks.
Currently, the folder hosts scripts for settin up your git pre-commit hook according to the standards of the project.

## Git Hooks

To use our custom git pre-commit hook that takes care of running clang-format. The minimum clang-format version necessary is quite old and will soon be updated to a newer version.

- python >= 3.8.0
- clang-format >= 8.0.0

To install the pre-commit hook, simply execute the `setup.py` script, which will take care of registering the current hook. Don't worry, if you already have a hook installed, the script will create a backup for you in the git hooks folder. Careful on successive runs though. The backup gets overwritten.

~~~bash
./Scripts/Format/setup.py
! pre-commit hook backup created at: 'D:\ExampleFolder\agux\.git\hooks\pre-commit.backup'
> Installing pre-commit hook at 'D:\ExampleFolder\agux\.git\hooks\pre-commit'
> Installed pre-commit hook at 'D:\ExampleFolder\agux\.git\hooks\pre-commit'
> Installation complete, you are ready to go.
~~~

In case you want to restore or keep a backup of your previous pre-commit hook, the first line tells you where to find the previous hook script.
As of this time, the hook does not check if clang-format is installed and will instead simply terminate on the first file it tries to format in case the command is not available or not in your path environment.

## Manual Formatting

To manually format all C++ header and source files (.h, .hpp, .cpp), invoke the `format.py` script. This script will recurisvely work through your **current working directory** and invoke clang-format on all C++ files. So make sure you use this from the right folder, or you will end up formatting third party plugins as well.