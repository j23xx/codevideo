#!/bin/bash
# debug still: ./shot.sh name "ex,ey,ez" "ax,ay,az" tau [extra args]
name=$1; eye=$2; at=$3; tau=$4; shift 4
./dronecam --w 960 --h 540 --spp 24 --out work/t --eye "$eye" --at "$at" --tau "$tau" "$@" 2>&1 | grep "^frame" | sed "s/^/$name: /"
mv work/t/f_00000.png work/t/$name.png
