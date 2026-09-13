import api
from api import LogLevel

def nothing(mod, instance_id: str) -> int:
    return 0

def produce_item(mod, instance_id: str, item_id: str) -> int:
    return api.set_resource(item_id, api.get_resource(item_id)+1)

def spawn_palace(mod, instance_id: str) -> int:
    x, y = api.get_pos(instance_id)
    owner = api.get_owner(instance_id)
    faction = api.get_faction(owner)

    api.delete_actor(instance_id)
    return api.spawn_actor(f"{faction}_palace", owner, x, y)

def remove_forest_tile(mod, instance_id: str):
    pass

def spawn_actor(mod, instance_id: str, actor_id: str, col: str, row: str) -> int:
    owner = api.get_owner(instance_id)
    actor_x, actor_y = api.get_pos(instance_id)
    if col == "$x$":
        x = actor_x
    else:
        x = int(col)
    if row == "$y$":
        y = actor_y
    else:
        y = int(row)
    return api.spawn_actor(actor_id, owner, x, y)

def delete_actor(mod, instance_id: str) -> int:
    return api.delete_actor(instance_id)