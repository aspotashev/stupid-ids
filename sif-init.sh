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
touch first_ids.txt
touch pot_names.txt
git add next_id.txt first_ids.txt pot_names.txt
git commit -m "init"

