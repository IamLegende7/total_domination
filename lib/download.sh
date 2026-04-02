#!/bin/sh

set -e

ARGUMENTS="$*"

cd $(dirname "$0")/..
cat .gitmodules | \
while true; do
    read module || break
    read line; set -- $line
    path=$3
    read line; set -- $line
    url=$3
    
    if [ -d "$path" ]; then
        if [ ! "$(ls -A $path)" ]; then
            git clone --filter=blob:none $url $path --recursive $ARGUMENTS
        else
            echo "Module $path already exists; skipping!"
        fi
    fi
done