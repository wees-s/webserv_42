#!/usr/bin/env python3
import datetime
import os
import sys
import json

# Use curriculum.json path
curriculum_path = "../data/curriculum.json"

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
    # File doesn't exist, return zeros
    date_formats = {
        "date": "00/00/0000",
        "time": "00:00:00",
        "datetime": "00/00/0000 00:00:00",
        "iso": "0000-00-00T00:00:00",
        "timestamp": "0"
    }

print(json.dumps(date_formats))