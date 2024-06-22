#ifndef __ENCODER_hpp__
#define __ENCODER_hpp__

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <linux/videodev2.h>
#include <linux/v4l2-controls.h>
#include <cstring>
#include <errno.h>
#include <cstdlib>

#include "buffer.hpp"


class Encoder 
{
public:
    Encoder(const std::string& outputFileName, int port, std::string& client);
    ~Encoder();
    bool setEncoderOption(unsigned int id, int value);
    bool mapBuffer(v4l2_buf_type type, struct VideoBuffer *buffer);
    bool initBuffer(enum v4l2_buf_type type, struct VideoBuffer* buffer);
    bool encodeFrame(unsigned char * buf, int size);
    bool destroyBuffer(VideoBuffer *buffer);
    bool setupInputBuffer(int width, int height, unsigned int format);
    bool setupOutputBuffer(int width, int height);
    void setFrameRate(enum v4l2_buf_type const type ,unsigned int framerate);
    void debug();
    void init(int width, int height, int bitrate, int fps, bool isSendEthernet = true, bool isWriteToFile = false, bool isUseSharedMemory = false, bool isFixedBirate= false);
    bool startStream(); 
    bool stopStream();

private:

    int fd;
    int channel;
    int socket;
    int outFile;
    struct v4l2_format inputFormat;
    struct v4l2_format outputFormat;
    // Allocate buffers

    sockaddr_in adderess;

    VideoBuffer inputBuffer;
    VideoBuffer outputBuffer;

    bool sendEthernet = false;
    bool writeToFile = false;
    bool useSharedMemory = false;

    camera_image_metadata_t meta;

    std::string serverName;
};

#endif //__ENCODER_hpp__