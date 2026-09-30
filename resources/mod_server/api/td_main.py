from api import communication
from logger import log, LogLevel
from pathlib import Path

def backend_main_log(log_level: LogLevel, message: str, mod_id: str) -> int:
    return int(communication.request_function("LOG", [log_level.value, message, mod_id]).get("status", 100))

def get_resource(resource_id: str) -> int:
    return int(communication.request_function("get_resource", [resource_id]).get("return", {}).get("count", -1))

def set_resource(resource_id: str, count: int) -> int:
    return int(communication.request_function("set_resource", [resource_id, count]).get("return", {}).get("status", 100))

def get_pos(actor_id: str) -> tuple:
    response = communication.request_function("get_pos", [actor_id]).get("return", {})
    return int(response.get("col", -1)), int(response.get("row", -1))

def get_owner(actor_id: str) -> int:
    return int(communication.request_function("get_owner", [actor_id]).get("return", {}).get("player_num", -1))

def get_faction(player_num: int) -> str:
    return str(communication.request_function("get_faction", [player_num]).get("return", {}).get("faction"))

def spawn_actor(actor_id: str, owner: int, col: int, row: int) -> int:
    return int(communication.request_function("spawn_actor", [actor_id, owner, col, row]).get("status", 100))

def delete_actor(actor_id: str) -> int:
    return int(communication.request_function("delete_actor", [actor_id]).get("status", 100))

def registry_add(category: str, key: str, value: Path) -> int:
    return int(communication.request_function("registry_add", [category, key, str(value)]).get("status", 100))

def registry_load(category: str, file: Path) -> int:
    return int(communication.request_function("registry_load", [category, str(file)]).get("status", 100))

def registry_get(category: str, key: str, default_value: Path=Path("")) -> int:
    return int(communication.request_function("registry_get", [category, key, str(default_value)]).get("return", {}).get("value"))