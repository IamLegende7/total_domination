import api
from api import LogLevel

def produce_item(mod, item_id: str) -> int:
    return api.set_resource(item_id, api.get_resource(item_id)+1)

def remove_forest_tile(mod, tile_x: int, tile_y: int):
    pass

def delete_actor(mod, actor_id: str):
    pass