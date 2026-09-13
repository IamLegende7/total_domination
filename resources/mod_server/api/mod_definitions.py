from logger import log as backend_log
from logger import LogLevel
from api.td_main import backend_main_log
import api

from pathlib import Path
from dataclasses import dataclass

@dataclass
class RegistryCache:
    category: str
    key: str # None if file
    value: Path # path to file or value

    def load(self) -> int:
        if self.key is None:
            status = api.registry_load(self.category, self.value)
        else:
            status = api.registry_add(self.category, self.key, self.value)
        return status
    

class Mod:
    ## Init ##
    def __init__(self, mod_id: str, name: str = None):
        self.id = mod_id
        self.name = name
        self.dir = Path(f"$mod_dir$/{self.id}")
        self.dependencies = []
        self.functions = {}
        self.registry = []

    def init(self):
        status = 0

        for registry_entry in self.registry:
            registry_status = registry_entry.load()
            status += registry_status
            if registry_status != 0:
                if registry_entry.key is None:
                    self.main_log(LogLevel.ERRR, "Could not load registry")
    
        if "init" in self.functions.keys():
            status += self.functions["init"](self)

        if status == 0:
            self.log(LogLevel.INFO, f"Loaded {self.id}")
            self.main_log(LogLevel.INFO, f"Loaded {self.id}")
            return 0
        else:
            self.log(LogLevel.ERRR, f"Failed to load {self.id}")
            self.main_log(LogLevel.ERRR, f"Failed to load {self.id}")
            return 1

    def add_dependency(self, mod_id: str) -> None:
        if (not mod_id in self.dependencies):
            self.dependencies.append(mod_id)

    def add_registry(self, category: str, key: str, value: Path) -> None:
        self.registry.append(RegistryCache(category, key, value))

    def add_registry_file(self, category: str, file: Path) -> None:
        self.registry.append(RegistryCache(category, None, file))

    ## Functions ##s
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