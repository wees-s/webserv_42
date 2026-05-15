#!/bin/bash

# Lista processos que estão usando a porta 8080
lsof -i :8080

# Mata o processo que está usando a porta 8080
kill -9 $(lsof -t -i :8080)

