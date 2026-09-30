# server.py
import sys, json
from pathlib import Path

from logger import log, LogLevel, init_logger

from api.internal import parse_line, handle_request

sys.path.insert(0, str(Path("resources/mod_server")))

if len(sys.argv) < 2:
    init_logger(Path("error.log"), LogLevel.INFO, True)
    log(LogLevel.CRIT, "Could not set log_file: missing argument(s): [log_file: str]")
    quit()
init_logger(Path(str(sys.argv[1])), LogLevel.DEBG, True)

for line in sys.stdin:
    request = parse_line(line)
    handle_request(request)