#!/usr/bin/env python

import pathlib
import subprocess
import sys


formatted_files = ['.cpp', '.h', '.hpp']
formatting_cmd = "clang-format -style=file -i"

path_root = pathlib.Path.cwd().absolute()

targets = [path_root.glob('**/*{}'.format(extension)) for extension in formatted_files]
targets = [file for sublist in targets for file in sublist]


for target in targets:
    print("> Formatting file '{}'".format(target.relative_to(path_root)))
    sys.stdout.flush()

    format_cmd = subprocess.Popen("{} {}".format(formatting_cmd, target.absolute()), shell=True, stderr=subprocess.PIPE)
    format_cmd.wait()

    # Check if formatting failed, and if so, abort the commit immediately and forward
    # the format.py output to the console. This allows users to fix this themselves if so desired.
    if format_cmd.returncode != 0:
        print("! Error: Formatting failed.", file=sys.stderr)
        print("! File: '{}' couldn't be formatted.".format(target.relative_to(path_root)), file=sys.stderr)
        print(file=sys.stderr)

        # Get all output from the standard error pipe from the formatting command, decode the byte-strings into utf-8
        # and redirect them to the pre-commit standard error before aborting with exit status 1.
        format_output = format_cmd.stderr.readlines()
        format_output = map(lambda line: line.decode('utf-8'), format_output)
        for line in format_output:
            print("   ! {}".format(line), file=sys.stderr, end='')
