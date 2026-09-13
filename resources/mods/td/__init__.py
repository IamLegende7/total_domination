from api import Mod
import api
from pathlib import Path
from . import test
from . import actors

mod = Mod("td", "Total Domination")

mod.add_registry_file("textures", Path("$resource_dir$/registry/textures.jsonc"))
mod.add_registry_file("actors", Path("$resource_dir$/registry/actors.jsonc"))
mod.add_registry_file("resources", Path("$resource_dir$/registry/resources.jsonc"))

mod.add_func(test.echo, "test_echo")
mod.add_func(test.logger, "test_logger")
mod.add_func(actors.defaults.nothing, "nothing")
mod.add_func(actors.defaults.produce_item, "produce_item")
mod.add_func(actors.defaults.spawn_palace, "spawn_palace")
mod.add_func(actors.defaults.remove_forest_tile, "remove_forest_tile")
mod.add_func(actors.defaults.spawn_actor, "spawn_actor")
mod.add_func(actors.defaults.delete_actor, "delete_actor")