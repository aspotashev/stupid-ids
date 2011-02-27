#!/bin/sh

DIR="$1"

if [ -z "$DIR" ]; then
	echo "Usage: ./sif-init.sh <dir>"
	exit
fi

mkdir -p "$DIR"
cd "$DIR"
git init

printf "100" > next_id.txt
touch hash_map.txt
touch pot_names.txt
git add next_id.txt hash_map.txt pot_names.txt
git commit -m "init"

