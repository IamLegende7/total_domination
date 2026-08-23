from api import communication
from logger import log, LogLevel

def backend_main_log(log_level: LogLevel, message: str, mod_id: str) -> int:
    return int(communication.request_function("LOG", [log_level.value, message, mod_id]).get("status"))

def get_resource(resource_id: str) -> int:
    return int(communication.request_function("get_resource", [resource_id]).get("count"))

def set_resource(resource_id: str, count: int) -> int:
    return int(communication.request_function("set_resource", [resource_id, count]).get("status"))