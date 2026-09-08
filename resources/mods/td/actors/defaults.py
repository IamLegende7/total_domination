import api
from api import LogLevel

def produce_item(mod, actor_id: str, item_id: str) -> int:
    return api.set_resource(item_id, api.get_resource(item_id)+1)

def spawn_palace(mod, actor_id: str):
    x, y = api.get_pos(actor_id)
    owner = api.get_owner(actor_id)
    faction = api.get_faction(owner)

    api.delete_actor(actor_id)
    return api.spawn_actor(f"{faction}_palace", owner, x, y)

def remove_forest_tile(mod, actor_id: str):
    pass

def delete_actor(mod, actor_id: str):
    return api.delete_actor(actor_id)