from pathlib import Path
import sys

from logger import log, LogLevel, init_logger

mods = {}

def execute_function(function: str, args: list) -> dict:
    if (function is None) or (len(function.split(":")) != 2):
        return {
            "status": 1,
            "return": {}
        }

    mod_str, func_str = function.split(":")
    if (mod_str == "" or func_str == ""):
        return {
            "status": 1,
            "return": {}
        }

    if mods.get(mod_str, None) is None:
        return {
            "status": 1,
            "return": {}
        }
    if not mods.get(mod_str).has_func(func_str):
        return {
            "status": 1,
            "return": {}
        }

    return_value = mods.get(mod_str).call_func(func_str, args)
    if return_value is None: return_value = {}
    return {
        "status": 0,
        "return": return_value
    }

def load_mod(path: Path) -> int:
    log(LogLevel.INFO, f"Loading mod \"{path.stem}\"..")
    if path is None:
        log(LogLevel.ERRR, f"Request data is missing one or more of [\"path\"]")
        return 1

    if (not path.parent.exists()) or (not path.parent.is_dir()):
        log(LogLevel.ERRR, f"path.parent either does not exist or isnt a directory: \"{path.parent}\"")
        return 1
    
    try:
        sys.path.insert(0, str(path.parent))
        module = __import__(path.stem)
        if not hasattr(module, "mod"):
            log(LogLevel.ERRR, f"Could not load mod \"{path}\": has no attribute \"mod\"")
            return 1

        mod = getattr(module, "mod")
        if not mods.get(mod.id, None) is None:
            log(LogLevel.WARN, f"Mod \"{mod.id}\" is already loaded")
            return 1

        mods[mod.id] = mod
        mod_status = mod.init()
        if mod_status != 0:
            log(LogLevel.WARN, f"Could not load mod \"{mod.id}\": \"{mod_status}\"")
            return 1
        else:
            log(LogLevel.INFO, f"Loaded mod \"{mod.id}\"")
            return 0
    except Exception as e:
        log(LogLevel.WARN, f"Could not load mod \"{mod_path}\": {type(e).__name__}: {e}")
        return 2