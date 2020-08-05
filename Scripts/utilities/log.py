import logging
from logging import *


def prepare():
    """
    Configures the logging output for default logs.
    """
    basicConfig(
        level=logging.INFO,
        format='%(asctime)s %(message)s',
        datefmt='%m/%d/%Y %I:%M:%S %p'
    )