
#ifndef __CAPTURE_hpp__
#define __CAPTURE_hpp__

#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <unistd.h>

#include <errno.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>
#include <linux/v4l2-controls.h>

#include "buffer.hpp"

#define NBUF 3


class Capture
{
private:
    unsigned char** buffer;
	int fd;
    int outFile;
	int size;
	int index;
	int nbufs;
    int count;
    bool writeToFile = false;

	fd_set fds;
public:
    Capture(const std::string& device, const std::string& outFileName, bool isWriteToFile = false);
    ~Capture();
    void attachBuffers(unsigned char **buffer);
    void queryCapabilites();
    int setFormat(int width, int height, int format);
    int requestBuffer();
    int queryBuffer(int index, unsigned char **buffer);
    int queueBuffer(int index);
    int startStreaming();
    void rotateImage(int angle);
    int dequeueBuffer();
    int stopStreaming();
    int readFrame();
    void setFrameRate(enum v4l2_buf_type const type ,unsigned int framerate);
};

#endif // __CAPTURE_hpp__