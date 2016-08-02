#!/bin/bash

UNAME=$(uname -s)
if [ "$UNAME" == "Darwin" ]; then
	ffmpeg -f qtkit -i "0" -vcodec libx264 -f h264 -pix_fmt yuv420p -r 30 -vf scale=640:480 http://localhost:8080/vstream?secret=1234
else
	if [ "$UNAME" == "Linux" ]; then
		ffmpeg -f -i /dev/video0 -vcodec libx264 -x264-params keyint=8 -f h264 -pix_fmt yuv420p -r 30 -vf scale=640:480 http://localhost:8080/vstream?secret=1234
	fi
fi