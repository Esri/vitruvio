from utilities import log
from utilities import paths
from pathlib import Path
from re import compile
from typing import Optional
from datetime import date

log = log.getLogger(__name__)
copyright_reminder = compile(r'^//\s*Fill out your copyright notice')
copyright_regex = compile(r'^//\s*Copyright\s*©?\s*'
                          r'(?P<start>\d{4})'
                          r'(\s*-\s*(?P<end>\d{4}))?\s*'
                          r'Esri( R&D Center Zurich)?\.\s*'
                          r'All [Rr]ights [Rr]eserved\.')
copyright_header = '// Copyright © 2017-{} Esri R&D Center Zurich. All rights reserved.\n\n'.format(date.today().year)


def parse_dates(line: str) -> Optional[int]:
    """
    Parses a copyright header line and returns the relevant (last) date.

    :param line: The line to parse.

    :return: None, if the line is not a valid copyright header.
    :return: The end date if the line is a date range.
    :return: The single date otherwise.
    """
    match = copyright_regex.match(line)

    if match is None:
        return None

    start = match.groupdict()['start']
    end = match.groupdict()['end']

    try:
        if start is not None:
            start = int(start)

        if end is not None:
            end = int(end)

    except ValueError:
        return None

    return end if end is not None else start


def fix_missing(path: Path, lines: [str], full_path=False):
    """
    Fixes a missing copyright header by prepending it.

    :param path: The path to the file under scrutiny.
    :param lines: The content of the file under scrutiny.
    :param full_path: If true, prints the full path of the file.
    """
    output = path.relative_to(paths.GLOBAL.PATH_ROOT) if full_path else path.name

    log.info('File {}: Could not find copyright notice. Adding a new one.'.format(output))

    if len(lines) > 0:
        header = lines[0]

        # Remove copyright reminder inserted by the Unreal Engine editor.
        # This copyright reminder only shows up when the file has been created through the editor.
        if copyright_reminder.match(header) is not None:
            log.info('File {}: Removing Unreal Editor copyright reminder.'.format(output))
            lines = lines[1:]
        elif 'copyright' in header or 'Copyright' in header:
            log.warning('File {}: Found an unknown copyright notice.'.format(output))
            log.warning('File {}: "{}"'.format(output, header.strip()))

        lines = [copyright_header] + lines

        with path.open('w', encoding='utf-8') as file:
            file.writelines(lines)


def fix_out_of_date(path: Path, lines: [str], full_path=False):
    """
    Fixes an out of date copyright header by replacing it with a new one.

    :param path: The path to the file under scrutiny.
    :param lines: The content of the file under scrutiny.
    :param full_path: If true, prints the full path of the file.
    """
    output = path.relative_to(paths.GLOBAL.PATH_ROOT) if full_path else path.name

    log.info('File {}: Copyright notice is out of date. Adding a new one.'.format(output))
    lines = [copyright_header] + lines[1:]

    with path.open('w', encoding='utf-8') as file:
        file.writelines(lines)


def fix(path: Path, full_path=False):
    """
    Checks the first line of a file for a copyright notice and if there is None,
    adds one.

    :param path: The path to the file to check.
    :param full_path: If true, prints the full path of the file.

    :return: False, if the file is not valid.
    :return: True, otherwise.
    """
    output = path.relative_to(paths.GLOBAL.PATH_ROOT) if full_path else path.name

    if not path.exists() or not path.is_file():
        log.warning('File {}: Can not edit copyright, file does not exist.'.format(output))
        return False

    if path.suffix not in ['.h', '.cpp', '.hpp']:
        log.warning('File {}: Can not edit copyright, file is not a C++ file.'.format(output))
        log.warning('File {}: Only ".h", ".hpp" and ".cpp" files are supported at the moment.'.format(output))
        return False

    # Read lines of the file.
    with path.open('r', encoding='utf-8-sig') as file:
        content = file.readlines()
        header = content[0].strip() if len(content) > 0 else ''

    # Prepare data necessary to make copyright decision.
    copyright_date = parse_dates(header)

    if copyright_date is None:
        fix_missing(path, content, full_path)
    elif date.today().year > copyright_date:
        fix_out_of_date(path, content, full_path)

    return True
