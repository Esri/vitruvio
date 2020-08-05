from utilities import log
from pathlib import Path
from re import compile
from typing import Optional


log = log.getLogger(__name__)
cpr = compile('^//\s*Copyright\s*©?\s*(?P<start>\d{4})(?P<end>-\d{4})? Esri (R&D Center Zurich)?\. All Rights Reserved\.')


def parse_dates(line: str) -> Optional[int]:
    """
    Parses a copyright header line and returns the relevant (last) date.

    :param line: The line to parse.

    :return: None, if the line is not a valid copyright header.
    :return: The end date if the line is a date range.
    :return: The single date otherwise.
    """
    match = cpr.match(line)

    if match is None:
        return None

    start = match.groupdict()['start']
    end = match.groupdict()['end']

    try:
        if start is not None:
            start = int(start)

        if end is not None:
            end = end[1:]
            end = int(end)

    except ValueError:
        log.warning('Could not parse start or end date.')
        log.warning('Start date: {}'.format(start))
        log.warning('End date: {}'.format(end))
        return None

    return end if end is not None else start


def fix(path: Path):
    """
    Checks the first line of a file for a copyright notice and if there is None,
    adds one.

    :param path: The path to the file to check.

    :return: False, if the file is not valid.
    :return: True, otherwise.
    """
    import datetime

    if not path.exists() or not path.is_file():
        log.warning('Can not edit copyright on path: "{}", file does not exist.'.format(path))
        return False

    if path.suffix not in ['.h', '.cpp', '.hpp']:
        log.warning('Can not edit copyright on path: "{}", file is not a C++ file.'.format(path))
        log.warning('Only ".h", ".hpp" and ".cpp" files are supported at the moment.')
        return False

    with path.open('r') as file:
        lines = file.readlines()
        first = lines[0] if len(lines) > 0 else ''

    with path.open('w') as file:
        date = parse_dates(first)
        header = '// Copyright © 2017-{} Esri R&D Center Zurich. All rights reserved.\n\n'.format(datetime.date.today().year)

        if date is None:
            log.info('Could not find copyright notice. Adding a new one.')
            lines = [header] + lines
            file.writelines(lines)

        elif datetime.date.today().year > date:
            log.info('Copyright notice is out of date. Adding a new one.')
            lines = [header] + lines[1:]
            file.writelines(lines)

    return True
