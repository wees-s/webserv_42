#!/usr/bin/env python3
import datetime
import os
import sys

print("Content-Type: application/json\r\n\r\n")

# Get PID from query string or use server PID
pid = os.environ.get('QUERY_STRING', '')
if not pid:
    # Fallback: try to get from SCRIPT_FILENAME or use default
    pid = str(os.getpid())

# Build path to curriculum.json
curriculum_path = f"www/users/user{pid}/curriculum.json"

try:
    # Get modification time of curriculum.json
    mtime = os.path.getmtime(curriculum_path)
    last_updated = datetime.datetime.fromtimestamp(mtime)
    
    date_formats = {
        "date": last_updated.strftime("%d/%m/%Y"),
        "time": last_updated.strftime("%H:%M:%S"),
        "datetime": last_updated.strftime("%d/%m/%Y %H:%M:%S"),
        "iso": last_updated.isoformat(),
        "timestamp": str(int(mtime))
    }
except OSError:
    # File doesn't exist, return current time
    now = datetime.datetime.now()
    date_formats = {
        "date": now.strftime("%d/%m/%Y"),
        "time": now.strftime("%H:%M:%S"),
        "datetime": now.strftime("%d/%m/%Y %H:%M:%S"),
        "iso": now.isoformat(),
        "timestamp": str(int(now.timestamp()))
    }

import json
print(json.dumps(date_formats))
