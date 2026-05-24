#!/usr/bin/env python3
# Script de teste para desagrupamento multipart/form-data
# Lê o stdin e imprime o conteúdo recebido

import os
import sys

print("Content-Type: application/json\r\n\r\n")

# Lê o stdin e imprime o conteúdo (para debug)
try:
    content = sys.stdin.read()
    content_type = os.environ.get('CONTENT_TYPE', 'unknown')
    
    # Parsear o formato: name=value ou name=filename:content
    fields = []
    for line in content.strip().split('\n'):
        if '=' in line:
            parts = line.split('=', 1)
            name = parts[0]
            value = parts[1]
            
            if value.startswith('file:'):
                # Formato: name=filename:content
                file_parts = value[5:].split(':', 1)
                filename = file_parts[0]
                file_content = file_parts[1] if len(file_parts) > 1 else ""
                fields.append(f'{{"name": "{name}", "type": "file", "filename": "{filename}", "content": "{file_content}"}}')
            else:
                # Formato: name=value
                fields.append(f'{{"name": "{name}", "type": "text", "value": "{value}"}}')
    
    if fields:
        print(f'{{"status": "ok", "content_type": "{content_type}"}}')
        for field in fields:
            print(field)
    else:
        print(f'{{"status": "ok", "content_type": "{content_type}", "fields": []}}')
except Exception as e:
    print(f'{{"error": "{str(e)}"}}')
