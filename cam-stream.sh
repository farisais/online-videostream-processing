#!/bin/bash

ffmpeg -f qtkit -i "0" -vcodec libx264 -x264-params keyint=8 -f h264 -pix_fmt yuv420p -r 30 -vf scale=640:480 http://localhost:8080/vstream?secret=1234