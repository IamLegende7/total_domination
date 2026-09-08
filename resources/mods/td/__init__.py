from api import Mod, LogLevel
import api
from pathlib import Path
from . import test
from . import actors

def init(mod):
    api.registry_load("textures", Path("$resource_dir$/registry/textures.jsonc"))
    api.registry_load("actors", Path("$resource_dir$/registry/actors.jsonc"))
    mod.main_log(LogLevel.INFO, f"Loaded {mod.id}")
    return 0

mod = Mod("td", "Total Domination")
mod.add_func(init)
mod.add_func(test.echo, "test_echo")
mod.add_func(test.logger, "test_logger")
mod.add_func(actors.defaults.produce_item, "produce_item")
mod.add_func(actors.defaults.spawn_palace, "spawn_palace")
mod.add_func(actors.defaults.remove_forest_tile, "remove_forest_tile")
mod.add_func(actors.defaults.delete_actor, "delete_actor")