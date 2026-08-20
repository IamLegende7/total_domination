# server.py
import sys, json
from pathlib import Path

from logger import log, LogLevel, init_logger

mods = {}
sys.path.insert(0, "resources/mod_server")

if len(sys.argv) < 2:
    init_logger(Path("error.log"), LogLevel.INFO, True)
    log(LogLevel.CRIT, "Could not set log_file: missing argument(s): [log_file: str]")
    quit()
init_logger(Path(str(sys.argv[1])), LogLevel.DEBG, True)

for line in sys.stdin:
    line = line.strip()
    if not line:
        continue
    request = json.loads(line)
    request_data = request.get("data")
    log(LogLevel.DEBG, f"Request: {request}")
    try:
        ## status ##
        if request.get("type") == "status":
            response = {
                "status": 0,
                "message": "Hello World!",
                "sender": "TDModServer",
                "data": {}
            }
        ## execute ##
        elif request.get("type") == "execute":
            mod_str = request_data.get("mod")
            func_str = request_data.get("function")
            if (mod_str is None) or (func_str is None):
                response = {
                    "status": 3,
                    "message": f"Request data is missing one or more of [\"mod\", \"function\"]",
                    "sender": "TDModServer",
                    "data": {}
                }
            elif not mod_str in mods.keys():
                response = {
                    "status": 1,
                    "message": f"Mod \"{mod_str}\" not loaded",
                    "sender": "TDModServer",
                    "data": {}
                }
            if not mods[mod_str].has_func(func_str):
                response = {
                    "status": 3,
                    "message": f"Mod \"{mod}\" does not include function \"{func_str}\"",
                    "sender": "TDModServer",
                    "data": {}
                }
            else:
                args = request_data.get("args")
                return_value = mods[mod_str].call_func(func_str, args)
                if return_value is None: return_value = {}
                response = {
                    "status": 0,
                    "message": f"Executed \"{mod_str}:{func_str}\"",
                    "sender": "TDModServer",
                    "data": {
                        "return": return_value
                    }
                }
        ## load_mod ##
        elif request.get("type") == "load_mod":
            mod_str = request_data.get("mod_path")
            if (mod_str is None):
                response = {
                    "status": 3,
                    "message": f"Request data is missing one or more of [\"mod_path\"]",
                    "sender": "TDModServer",
                    "data": {}
                }
            else:
                mod_path = Path(mod_str)
                if (not mod_path.parent.exists()) or (not mod_path.parent.is_dir()):
                    response = {
                        "status": 1,
                        "message": f"mod_path.parent either does not exist or isnt a directory: \"{mod_path.parent}\"",
                        "sender": "TDModServer",
                        "data": {}
                    }
                else:
                    try:
                        sys.path.insert(0, str(mod_path.parent))
                        module = __import__(str(Path(mod_path.stem)))
                        if not hasattr(module, "mod"):
                            response = {
                                "status": 4,
                                "message": f"Could not load mod \"{mod_path}\": has no attribute \"mod\"",
                                "sender": "TDModServer",
                                "data": {}
                            }
                        else:
                            mod = getattr(module, "mod")
                            if mod.id in mods.keys():
                                response = {
                                    "status": 1,
                                    "message": f"Mod \"{mod.id}\" is already loaded",
                                    "sender": "TDModServer",
                                    "data": {}
                                }
                            else:
                                mods[mod.id] = mod
                                log(LogLevel.INFO, f"Loaded mod \"{mod.id}\"")
                                mod_status = mod.init()
                                if mod_status != 0:
                                    response = {
                                        "status": 1,
                                        "message": f"Could not load mod \"{mod.id}\": \"{mod_status}\"",
                                        "sender": "TDModServer",
                                        "data": {}
                                    }
                                else:
                                    response = {
                                        "status": 0,
                                        "message": f"Loaded mod \"{mod.id}\"",
                                        "sender": "TDModServer",
                                        "data": {}
                                    }
                    except Exception as e:
                        response = {
                            "status": 3,
                            "message": f"Could not load mod \"{mod_path}\": {type(e).__name__}: {e}",
                            "sender": "TDModServer",
                            "data": {}
                        }
        ## unknown ##
        else:
            response = {
                "status": 2,
                "message": f"Unknown Request type \"{request.get("type")}\"",
                "sender": "TDModServer",
                "data": {}
            }

    except Exception as e:
        response = {
            "status": 1,
            "message": f"Unhandled exception: {type(e).__name__}: {e}",
            "sender": "TDModServer",
            "data": {}
        }

    log(LogLevel.DEBG, f"   Response: {response}")
    sys.stdout.write(json.dumps(response) + "\n")
    response = {}
    sys.stdout.flush()