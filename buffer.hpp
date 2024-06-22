#ifndef __BUFFER_hpp__
#define __BUFFER_hpp__


#include <linux/videodev2.h>
#include <string>
#include "utils.hpp"

struct VideoBuffer {
    void* start;
    int length;

    struct v4l2_buffer inner;
    struct v4l2_plane plane;
};

struct CaptureBuffer {
    void* start;
    int length;
    int index;
};



#endif // __BUFFER_hpp__