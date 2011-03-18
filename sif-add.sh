#!/bin/sh
#
# Adds to a Git repository:
#  1. the given .pot file
#  2. the first ID for that .pot file
#  3. map: [sha1 of .pot] <-> [name and date of .pot]

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

if [ -z "$(grep --fixed-strings $POT_HASH first_ids.txt)" ]; then
	NEXT_ID=$(cat next_id.txt)
	POT_LEN=$(python ../get_pot_length.py "$POT")
	NEXT_ID2=$(echo "$NEXT_ID + $POT_LEN" | bc)

	echo "$POT_HASH $NEXT_ID" >> first_ids.txt
	printf $NEXT_ID2 > next_id.txt

	git add first_ids.txt next_id.txt
else
	echo "The .pot file already exists in this repository."
fi

POT_DATE=$(python ../get_pot_date.py "$POT")
POT_NAMES_LINE="$POT_HASH $POT_NAME <$POT_DATE>"
if [ -z "$(grep --fixed-strings "$POT_NAMES_LINE" pot_names.txt)" ]; then
	echo "$POT_NAMES_LINE" >> pot_names.txt
	git add pot_names.txt
fi

git commit -m "add $POT_NAME ($POT_DATE)"

