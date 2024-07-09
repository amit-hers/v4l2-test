# rpi-v4l2-m2m-example
Raspberry pi v4l2 m2m encoder example

The application shows incorrect work of the bcm2835-codec encoder on some resolutions. Discussion here - https://forums.raspberrypi.com/viewtopic.php?p=2183718

# Build steps
```
- sudo apt install libv4l-dev build-essential cmake
- make
```

# Testing
  For play the raw h264 frames you can use gstreamer
  ```
gst-launch-1.0 filesrc location=/home/amither/output.raw ! rawvideoparse format=nv12 width=320 height=240 framerate=1/1 'plane-strides=<320,320>' 'plane-offsets=<0,76800>' ! videoconvert ! autovideosink

gst-launch-1.0 filesrc location=/home/amither/encode.h264 ! h264parse ! avdec_h264 ! xvimagesink

sudo gst-launch-1.0 udpsrc port=4000 ! queue ! h264parse ! avdec_h264 ! xvimagesink

  ```
