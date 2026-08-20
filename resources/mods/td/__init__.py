from api import Mod, LogLevel
from . import test

def init(mod):
    mod.main_log(LogLevel.INFO, f"Loaded {mod.id}")
    return 0

mod = Mod("td", "Total Domination")
mod.add_func(init)
mod.add_func(test.echo, "test_echo")
mod.add_func(test.logger, "test_logger")