# online-videostream-processing
Experimenting online video stream processing which include transcoding, image processing (face recognition, corner detection, etc) in realtime, instead performing the frame processing locally

HTTP and websocket server are written in GO, video decoding and encoding library are written in C utilizing ffmpeg (libavcoded, libavutil, and libswscale), image processing library using OpenCV with python binding, and web client decoder to view the processed video is using jsmpeg library which written in javascript.

Video will be streamed using h264 to http server, decoded to PPM (portable pixmap format) and processed. The processed image then encoded to MPEG1 and broadcasted through websocket connection to connected client(s). 

## Dependencies
This project using several dependencies as follows:
- [Go](https://golang.org/) version >1.5
- [ffmpeg](http://ffmpeg.org)
- [jsmpeg](https://github.com/phoboslab/jsmpeg)
- [OpenCV](http://opencv.org/) version >3

## How To Install
Make sure that you have installed Golang, ffmpeg and OpenCV locally. 

You can find the steps to compile and install ffmpeg in this link <https://trac.ffmpeg.org/wiki/CompilationGuide>.

I'm using anaconda to install OpenCV on my machine <https://www.continuum.io/downloads> and run 'conda install opencv'. Or you can download form http://opencv.org/downloads.html

To build and install online-videostream-processing:
```
make
make install
make clean
```
## How To Run
To run the server you need to specify two options which are the codec (to decode the frame) and the processing algorithm. Available algorithm:
- face : face detection
- corner_detect : corner detection
- background_substract_mog2 : background substractor
- edge_detect : edge detection

If you have your own algorithm just put under "processing" folder and specify its filename in the --alg option (without .py extension)
```shell
# sample using h264 codec to decode and face detection algorithm
./server --codec=h264 --alg=face

# using mpeg1 codec to decode and corner detection algorithm
./server --codec=mpeg1video --alg=corner_detect
```
Open cam and stream to endpoint.
```
./cam-stream.sh
```

\* Note that you need to modify codec paramter manually in the ffmpeg option according to the one you specify in the server argument.

Open client.html to see the live stream of the processed image
