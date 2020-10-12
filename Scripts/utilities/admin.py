import os
import traceback
from utilities import log


log = log.getLogger(__name__)


# noinspection PyBroadException
def is_admin():
    """
    Checks if the user is an admin.

    :return: True, if the user is in admin mode.
    :return: False, if the user is not in admin mode.
    :return: False, if the admin mode could not be checked.
    """
    if os.name == 'nt':
        import ctypes
        try:
            return ctypes.windll.shell32.IsUserAnAdmin()
        except Exception as e:
            traceback.print_exc()
            return False

    elif os.name == 'posix':
        # Check for root on Posix
        return os.getuid() == 0

    else:
        return False
