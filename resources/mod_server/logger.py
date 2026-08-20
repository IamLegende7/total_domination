"""
 - init_logger(log_file: str = 'app.log', log_level: LogLevel = LogLevel.INFO, clear_log_file: bool = True) -> None
 - LogLevel(Enum)
 - log(log_level: LogLevel, msg: str, colour: bool = True) -> bool
"""

from enum import Enum
from time import strftime

class LogLevel(Enum):
    """
        DEBG = 0    : DEBUG;    Informationen fürs Debugging
        INFO = 1    : INFO;     Generelle Information; alles normal
        WARN = 2    : WARNING;  Fehler/Problem, keine größeren Auswirkungen
        ERRR = 3    : ERROR;    Fehler, größere Auswirkungen
        CRIT = 4    : CRITICAL; Kritischer Fehler, App kann nicht weiter laufen
    """
    DEBG = 0
    INFO = 1
    WARN = 2
    ERRR = 3
    CRIT = 4

def init_logger(log_file: str = 'app.log', log_level: LogLevel = LogLevel.INFO, clear_log_file: bool = True) -> None:
    global log_file_setting
    global log_level_setting

    log_file_setting = log_file
    log_level_setting = log_level

    if clear_log_file:
        open(log_file, 'w').close()

_prefixes = {
    LogLevel.DEBG: "DEBUG:",
    LogLevel.INFO: "INFO:",
    LogLevel.WARN: "WARNING:",
    LogLevel.ERRR: "ERROR:",
    LogLevel.CRIT: "CRITICAL:"
}

_prefixes_colour = {
    LogLevel.DEBG: "\033[32;1mDEBUG\033[0m:",
    LogLevel.INFO: "\033[34;1mINFO\033[0m:",
    LogLevel.WARN: "\033[33;1mWARNING\033[0m:",
    LogLevel.ERRR: "\033[31;1mERROR\033[0m:",
    LogLevel.CRIT: "\033[37;41;1mCRITICAL\033[0m:"
}

def log(log_level: LogLevel, msg: str, colour: bool = True) -> bool:
    """
        Diese Funktion loggt ein Text zu einer Logfile.

        ---

        Args:
            log_level: LogLevel
            msg: str
            colour: bool            : ob ANSI escape codes für Farben in stdout benutzt werden sollen
        Returns:
            result: bool            : True wenn es keine Probleme gab
        ---
    """

    if not (log_level_setting is None) and log_level_setting.value > log_level.value:
        return True
    else:
        timestamp = strftime('[%Y-%m-%d %H:%M:%S]')
        if log_file_setting is None:
            log(LogLevel.ERRR, 'No log file set!', to_file=False)
        else:
            with open(log_file_setting, 'a') as file:
                file.write(f'{timestamp} {_prefixes[log_level]} {msg}\n')

        return True