#!/bin/sh
g++ main.cpp dbuslokalize.cpp -ldbus-glib-1 -lgettextpo_helper -I/usr/include/dbus-1.0 -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -o dbuslokalize
