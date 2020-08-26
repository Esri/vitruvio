from logging import *
import platform


def add_coloring_to_emit_windows(fn):
    # add methods we need to the class
    def _out_handle(self):
        import ctypes
        return ctypes.windll.kernel32.GetStdHandle(self.STD_OUTPUT_HANDLE)

    out_handle = property(_out_handle)

    def _set_color(self, code):
        import ctypes
        # Constants from the Windows API
        self.STD_OUTPUT_HANDLE = -11
        hdl = ctypes.windll.kernel32.GetStdHandle(self.STD_OUTPUT_HANDLE)
        ctypes.windll.kernel32.SetConsoleTextAttribute(hdl, code)

    setattr(StreamHandler, '_set_color', _set_color)

    # noinspection PyPep8Naming
    def new(*args):
        FOREGROUND_BLUE = 0x0001  # text color contains blue.
        FOREGROUND_GREEN = 0x0002  # text color contains green.
        FOREGROUND_RED = 0x0004  # text color contains red.
        FOREGROUND_INTENSITY = 0x0008  # text color is intensified.
        FOREGROUND_WHITE = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED
        # winbase.h
        STD_INPUT_HANDLE = -10
        STD_OUTPUT_HANDLE = -11
        STD_ERROR_HANDLE = -12

        # wincon.h
        FOREGROUND_BLACK = 0x0000
        FOREGROUND_BLUE = 0x0001
        FOREGROUND_GREEN = 0x0002
        FOREGROUND_CYAN = 0x0003
        FOREGROUND_RED = 0x0004
        FOREGROUND_MAGENTA = 0x0005
        FOREGROUND_YELLOW = 0x0006
        FOREGROUND_GREY = 0x0007
        FOREGROUND_INTENSITY = 0x0008  # foreground color is intensified.

        BACKGROUND_BLACK = 0x0000
        BACKGROUND_BLUE = 0x0010
        BACKGROUND_GREEN = 0x0020
        BACKGROUND_CYAN = 0x0030
        BACKGROUND_RED = 0x0040
        BACKGROUND_MAGENTA = 0x0050
        BACKGROUND_YELLOW = 0x0060
        BACKGROUND_GREY = 0x0070
        BACKGROUND_INTENSITY = 0x0080  # background color is intensified.

        level = args[1].levelno
        if level >= 50:
            color = BACKGROUND_YELLOW | FOREGROUND_RED | FOREGROUND_INTENSITY | BACKGROUND_INTENSITY
        elif level >= 40:
            color = FOREGROUND_RED | FOREGROUND_INTENSITY
        elif level >= 30:
            color = FOREGROUND_YELLOW | FOREGROUND_INTENSITY
        elif level >= 20:
            color = FOREGROUND_GREEN
        elif level >= 10:
            color = FOREGROUND_MAGENTA
        else:
            color = FOREGROUND_WHITE
        args[0]._set_color(color)

        ret = fn(*args)
        args[0]._set_color(FOREGROUND_WHITE)
        # print "after"
        return ret

    return new


def add_coloring_to_emit_ansi(fn):
    # add methods we need to the class
    def new(*args):
        level = args[1].levelno
        if level >= 50:
            color = '\x1b[31m'  # red
        elif level >= 40:
            color = '\x1b[31m'  # red
        elif level >= 30:
            color = '\x1b[33m'  # yellow
        elif level >= 20:
            color = '\x1b[32m'  # green
        elif level >= 10:
            color = '\x1b[35m'  # pink
        else:
            color = '\x1b[0m'  # normal
        args[1].msg = color + args[1].msg + '\x1b[0m'  # normal
        # print "after"
        return fn(*args)

    return new


if platform.system() == 'Windows':
    # Windows does not support ANSI escapes and we are using API calls to set the console color
    StreamHandler.emit = add_coloring_to_emit_windows(StreamHandler.emit)
else:
    # all non-Windows platforms are supporting ANSI escapes so we use them
    StreamHandler.emit = add_coloring_to_emit_ansi(StreamHandler.emit)


def prepare():
    """
    Configures the logging output for default logs.
    """
    basicConfig(
        level=INFO,
        format='%(asctime)s %(message)s',
        datefmt='%m/%d/%Y %I:%M:%S %p'
    )
