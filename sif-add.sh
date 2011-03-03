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
	NEXT_ID=$(cat next_id.txt)
	POT_LEN=$(../get_pot_length.py "$POT")
	NEXT_ID2=$(echo "$NEXT_ID + $POT_LEN" | bc)

	echo "$POT_HASH $NEXT_ID" >> hash_map.txt
	printf $NEXT_ID2 > next_id.txt

	git add hash_map.txt next_id.txt
else
	echo "The .pot file already exists in this repository."
fi

POT_DATE=$(../get_pot_date.py "$POT")
POT_NAMES_LINE="$POT_HASH $POT_NAME <$POT_DATE>"
if [ -z "$(grep --fixed-strings "$POT_NAMES_LINE" pot_names.txt)" ]; then
	echo "$POT_NAMES_LINE" >> pot_names.txt
	git add pot_names.txt
fi

git commit -m "add $POT_NAME ($POT_DATE)"

