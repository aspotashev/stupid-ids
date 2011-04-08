#!/bin/sh
#
# Creates a Git repository for adding .pot files and info about IDs.

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
touch pot_origins.txt
git add next_id.txt first_ids.txt pot_origins.txt
git commit -m "init"
git tag init # this tag is necessary for update-database.rb

