##ClientExperiments-Android (ffmpeg branch)
This repository includes experiments in recording, encoding and processing audio and video for streaming to the OpenWatch MediaCapture server.


###Outcome
Software encoding with FFmpeg is not a viable solution to recording two simultaneous video streams. Encoding in software and traversing the JNI present significant performance hurdles.

###Capabilities

This is a proof-of-concept for live encoding of two video streams using ffmpeg on the Android platform. 

### Performance
Simultaneously recording 640x480 and 320x240 video streams (via a single CameraPreviewCallback @ 640x480) bogged the test device (Galaxy Nexus) down to ~2fps.

###Inspiration

http://dmitrydzz-hobby.blogspot.com/2012/04/how-to-build-ffmpeg-and-use-it-in.html
https://github.com/havlenapetr/FFMpeg