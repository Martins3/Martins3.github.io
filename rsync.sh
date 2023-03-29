#!/usr/bin/env bash
# --human-readable, -h     output numbers in a human-readable format
rsync -avzh --delete --filter="dir-merge,- .gitignore" "$(pwd)/"  . martins3@10.0.0.2:"$(pwd)/"
# 需求: @todo 两侧都可以执行
