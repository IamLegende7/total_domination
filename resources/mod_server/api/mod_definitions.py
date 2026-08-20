from logger import log as backend_log
from logger import LogLevel
from api.td_main import backend_main_log

class Mod:
    def __init__(self, mod_id: str, name: str = None):
        self.id = mod_id
        self.name = name
        self.dependencies = []
        self.functions = {}

    def init(self):
        self.log(LogLevel.INFO, f"Loaded {self.id}")
        if "init" in self.functions.keys():
            return self.functions["init"](self)
        else:
            return 0

    def add_dependency(self, mod_id: str) -> None:
        if (not mod_id in self.dependencies):
            self.dependencies.append(mod_id)

    def add_func(self, function, custom_key: str = None) -> None:
        if custom_key is None:
            self.functions[function.__name__] = function
        else:
            self.functions[custom_key] = function

    def has_func(self, name: str) -> bool:
        return name in self.functions.keys()

    def call_func(self, name: str, args: list = None):
        if name in self.functions.keys():
            if args is None:
                return self.functions[name](self)
            else:
                return self.functions[name](self, *args)
        else:
            self.log(LogLevel.WARN, f"Function \"{name}\" not found in \"{self.id}\"")

    def log(self, log_level: LogLevel, msg: str, colour: bool = True):
        backend_log(log_level, f"[{self.id}] {msg}", colour)

    def main_log(self, log_level: LogLevel, message: str):
        backend_main_log(log_level, message, self.id)