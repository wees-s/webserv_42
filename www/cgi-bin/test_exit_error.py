#!/usr/bin/env python3
# Script de teste com exit code != 0
print("Content-Type: application/json\r\n\r\n")
print('{"status": "error"}')
exit(1)
