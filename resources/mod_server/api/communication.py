from logger import log, LogLevel
import sys
import json

def request_function(function: str, args: list) -> dict:
    request = {
        "status": -1,
        "message": f"Requesting function \"{function}\"",
        "sender": "ModdingAPI",
        "data": {
            "function": function,
            "args": args
        }
    }

    log(LogLevel.DEBG, f"    - Func request: {request}")
    sys.stdout.write(json.dumps(request) + "\n")
    sys.stdout.flush()

    for line in sys.stdin:
        line = line.strip()
        if not line:
            continue
        response = json.loads(line)
        if response.get("type") != "requested_function_response":
            log(LogLevel.WARN, f"         Type is \"{response.get("type")}\" (not \"requested_function_response\")")
            continue
        log(LogLevel.DEBG, f"         Response: {response}")
        return response.get("data")