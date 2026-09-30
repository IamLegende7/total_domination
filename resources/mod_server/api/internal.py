from logger import log, LogLevel
import sys
import json
from dataclasses import dataclass
from enum import Enum
from uuid import UUID
from pathlib import Path
import traceback

from api.communication import send, send_response
from api.mod_handling import execute_function, load_mod

class RequestType(Enum):
    EXECUTE = 0
    LOAD_MOD = 1
    STATUS = 2
    RESPONSE = 3
    ERROR = 4 # Internal use only

@dataclass
class Request:
    uuid: UUID
    request_type: RequestType
    origin: bool # True = TDModServer, False = TD
    data: dict

    def __str__(self):
        return f"Request(id=\"{str(self.uuid)}\", type=RequestType.{self.request_type.name}, origin={"\"TDModServer\"" if self.origin else "\"TD\""}, data={self.data})"


STATUS = -1
STATUS_MSGS = {
    -1: "TDModServer not loaded right now!",
    0: "TDModServer is loaded; ready to handle requests, sir!",
    1: "Something went wrong, sorry!"
}

def parse_line(line: str) -> Request:
    line = line.strip()
    if not line:
        return Request(None, RequestType.ERROR, None, {})
    request_json = json.loads(line)

    uuid = UUID(request_json.get("id", "00000000-0000-4000-0000-000000000000"))
    if uuid is None:
        log(LogLevel.WARN, f"Request {request_json} has no \"id\" member.")
        return Request(None, RequestType.ERROR, None, {})

    request_type = None
    if request_json.get("type", None) is None:
        log(LogLevel.WARN, f"Request {request_json} has no \"type\" member.")
        return Request(None, RequestType.ERROR, None, {})
    elif not hasattr(RequestType, request_json.get("type").upper()):
        log(LogLevel.WARN, f"Request {request_json}: \"type\" has no valid value: \"{request_json.get("type")}\".")
        return Request(None, RequestType.ERROR, None, {})
    else:
        request_type = getattr(RequestType, request_json.get("type").upper())

    if request_json.get("origin", None) is None:
        log(LogLevel.WARN, f"Request {request_json} has no \"origin\" member.")
        return Request(None, RequestType.ERROR, None, {})
    elif not request_json.get("origin") in ["TDModServer", "TD"]:
        log(LogLevel.WARN, f"Request {request_json}: \"origin\" has no valid value: \"{request_json.get("origin")}\".")
        return Request(None, RequestType.ERROR, None, {})
    
    request = Request(
        uuid,
        request_type,
        (request_json.get("origin") == "TDModServer"),
        request_json.get("data", {})
    )

    log(LogLevel.DEBG, f"Recieved: {line}")
    #log(LogLevel.DEBG, f"Parsed:   {request}")
    return request

def handle_request(request: Request) -> None:
    try:
        if request.request_type == RequestType.EXECUTE:
            data = execute_function(request.data.get("function", None), request.data.get("args", []))
            send_response(request.uuid, data.get("status", 2), data.get("return", {}))
            return None
        elif request.request_type == RequestType.LOAD_MOD:
            status = load_mod(Path(request.data.get("path", None)))
            send_response(request.uuid, status, {})
            return None
        elif request.request_type == RequestType.STATUS:
            send_response(request.uuid, STATUS, {"msg": STATUS_MSGS.get(STATUS, "... I don't know this status code...")})
            return None
        elif request.request_type == RequestType.RESPONSE:
            # This should not be executed; Responses with the right, expected uuid are handled before this function.
            # TODO: log an error
            return None
    except Exception as e:
        log(LogLevel.CRIT, "HELP!")
        log(LogLevel.CRIT, traceback.format_exc())
        send_response(request.uuid, -1, {"msg": f"{type(e).__name__}: {e}"})
        return None