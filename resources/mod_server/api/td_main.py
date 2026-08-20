from api import communication
from logger import log, LogLevel

def backend_main_log(log_level: LogLevel, message: str, mod_id: str) -> int:
    return int(communication.request_function("LOG", [log_level.value, message, mod_id]).get("status"))