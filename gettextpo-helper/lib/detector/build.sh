#!/bin/sh
g++ test.cpp detectorbase.cpp gitloader.cpp xstrdup.cpp processorphans.cpp -lgit2 -ggdb -o test
