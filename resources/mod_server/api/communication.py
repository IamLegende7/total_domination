import sys
import json
from uuid import UUID, uuid4
from pathlib import Path

from logger import log, LogLevel

def send(uuid: UUID, payload_type: str, data: dict) -> None:
    response = {
        "id": str(uuid),
        "type": payload_type,
        "origin": "TDModServer",
        "data": data
    }
    log(LogLevel.DEBG, f"Send:     {response}")
    sys.stdout.write(json.dumps(response) + "\n")
    sys.stdout.flush()

def send_response(uuid: UUID, status: int, return_value: dict = {}) -> None:
    data = {
        "status": status,
        "return": return_value
    }
    send(uuid, "response", data)

from api.internal import parse_line, handle_request, RequestType

def request_function(function: str, args: list) -> dict:
    data = {
        "function": function,
        "args": args
    }
    uuid = uuid4()
    send(uuid, "execute", data)

    i = 0
    for line in sys.stdin:
        if not line:
            continue
        request = parse_line(line)
        if (request.request_type == RequestType.RESPONSE) and (request.uuid == uuid):
            return request.data
        else:
            handle_request(request)
            if i >= 1:
                log(LogLevel.WARN, "Exiting request_function loop to prevent freezing.")
                return {
                    "status": -1,
                    "return": {}
                }
            i += 1