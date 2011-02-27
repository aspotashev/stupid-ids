#!/bin/sh

DIR="$1"
POT="$2"
POT_NAME="$3"

if [ -z "$DIR" ]; then
	echo "Usage: ./sif-init.sh <dir> <.pot> <.pot name>"
	exit
fi
if [ -z "$POT" ]; then
	echo "Usage: ./sif-init.sh <dir> <.pot> <.pot name>"
	exit
fi
if [ -z "$POT_NAME" ]; then
	echo "Usage: ./sif-init.sh <dir> <.pot> <.pot name>"
	exit
fi

cd "$DIR"
POT_HASH=$(git-hash-object "$POT")

if [ -z "$(grep --fixed-strings $POT_HASH hash_map.txt)" ]; then
	cp "$POT" "./.file.po"

	NEXT_ID=`cat next_id.txt`
	NEXT_ID2=$(posieve.py fill_xx_numbering ./.file.po -snext_id:$NEXT_ID --quiet)
	POIDS_HASH=`git-hash-object .file.po`

	echo "$POT_HASH $POIDS_HASH $POT_NAME" >> hash_map.txt
	printf $NEXT_ID2 > next_id.txt

	git add .file.po hash_map.txt next_id.txt
	git commit -m "new .pot: $POT_NAME"
else
	echo "The .pot file already exists in this repository."
fi

