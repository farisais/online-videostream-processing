# online-videostream-processing
I'm testing an online processing video stream which include transcoding, image processing (face recognition) in realtime. 

HTTP and websocket server are written in GO, video decoding and encoding library are written in C utilizing ffmpeg (libavcoded, libavutil, and libswscale), image processing library using OpenCV with python binding, and web client decoder to view the processed video is using jsmpeg library which written in javascript.

## Dependencies
This project using several dependencies as follows:
- ffmpeg
- jsmpeg
- opencv version >3

## How To Build

## How To Run
