import api
from logger import LogLevel, log

def echo(mod, value = "None"):
    return {"value": value}

def logger(mod):
    mod.main_log(LogLevel.DEBG, "Testing logger from Mod.")
    mod.main_log(LogLevel.INFO, "Testing logger from Mod.")
    mod.main_log(LogLevel.WARN, "Testing logger from Mod.")
    mod.main_log(LogLevel.ERRR, "Testing logger from Mod.")
    mod.main_log(LogLevel.CRIT, "Testing logger from Mod.")