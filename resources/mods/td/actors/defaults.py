from api import LogLevel

def produce_item(mod, item_id: str):
    mod.main_log(LogLevel.DEBG, f"Adding resource \"{item_id}\"")

def remove_forest_tile(mod, tile_x: int, tile_y: int):
    pass

def delete_actor(mod, actor_id: str):
    pass