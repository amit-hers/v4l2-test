#include "encoder.hpp"
#include "capture.hpp"
#include "utils.hpp"

#include <string>
#include <thread>

#define NUM_BUFFERS 300


struct Paramters 
{
    std::string devicePath;
    std::string outputFileName;
    int width;
    int height;
    int fps;
    int lowBitrate;
    int highBitrate;
    int framerate;
    int basePort;
    std::string address;
    bool isBuggy;
};




void* mainloop(void* params)
{
    Paramters* videoParams = (Paramters*)params;

    // HDMI_TO_MIPI reset bug.
    if(videoParams->isBuggy)
    {
        fixBug();
    }

    unsigned char *buffer[3];

    int index;
    int size;

    std::string highName = videoParams->outputFileName + std::string("_high");
    std::string lowName = videoParams->outputFileName + std::string("_low");

    Encoder encoder(highName, videoParams->basePort, videoParams->address);
    Encoder encoder1(lowName, videoParams->basePort+1, videoParams->address);
    
    Capture capture(videoParams->devicePath, videoParams->outputFileName);

    capture.attachBuffers(buffer);

    encoder.setEncoderOption(V4L2_CID_MPEG_VIDEO_BITRATE, videoParams->highBitrate);
    encoder.setEncoderOption(V4L2_CID_MPEG_VIDEO_H264_I_PERIOD, videoParams->fps);
    encoder.setEncoderOption(V4L2_CID_MPEG_VIDEO_H264_PROFILE, V4L2_MPEG_VIDEO_H264_PROFILE_CONSTRAINED_BASELINE);
    encoder.setEncoderOption(V4L2_CID_MPEG_VIDEO_H264_LEVEL, V4L2_MPEG_VIDEO_H264_LEVEL_4_0);
    encoder.setEncoderOption(V4L2_CID_MPEG_VIDEO_REPEAT_SEQ_HEADER, 1);
    // encoder.setEncoderOption(V4L2_CID_MPEG_VIDEO_H264_MIN_QP, 2);
    // encoder.setEncoderOption(V4L2_CID_MPEG_VIDEO_H264_MAX_QP, 8);

    encoder.setupInputBuffer(videoParams->width, videoParams->height, V4L2_PIX_FMT_NV12);
    encoder.setupOutputBuffer(videoParams->width, videoParams->height);

    encoder.setFrameRate(V4L2_BUF_TYPE_VIDEO_OUTPUT, 30);
    encoder.setFrameRate(V4L2_BUF_TYPE_VIDEO_CAPTURE, 30);

    encoder.startStream();

    encoder.debug();


    
    encoder1.setEncoderOption(V4L2_CID_MPEG_VIDEO_BITRATE, videoParams->lowBitrate);
    encoder1.setEncoderOption(V4L2_CID_MPEG_VIDEO_H264_I_PERIOD, videoParams->fps);
    encoder1.setEncoderOption(V4L2_CID_MPEG_VIDEO_H264_PROFILE, V4L2_MPEG_VIDEO_H264_PROFILE_CONSTRAINED_BASELINE);
    encoder1.setEncoderOption(V4L2_CID_MPEG_VIDEO_H264_LEVEL, V4L2_MPEG_VIDEO_H264_LEVEL_4_0);
    encoder1.setEncoderOption(V4L2_CID_MPEG_VIDEO_REPEAT_SEQ_HEADER, 1);
    // encoder1.setEncoderOption(V4L2_CID_MPEG_VIDEO_H264_MIN_QP, 2);
    // encoder1.setEncoderOption(V4L2_CID_MPEG_VIDEO_H264_MAX_QP, 8);

    encoder1.setupInputBuffer(videoParams->width, videoParams->height, V4L2_PIX_FMT_NV12);
    encoder1.setupOutputBuffer(videoParams->width, videoParams->height);

    encoder1.setFrameRate(V4L2_BUF_TYPE_VIDEO_OUTPUT, 30);
    encoder1.setFrameRate(V4L2_BUF_TYPE_VIDEO_CAPTURE, 30);

    encoder1.startStream();

    encoder1.debug();

	capture.queryCapabilites();

	capture.setFormat(videoParams->width, videoParams->height, V4L2_PIX_FMT_NV12);

    capture.setFrameRate(V4L2_BUF_TYPE_VIDEO_CAPTURE, videoParams->framerate);

	size = capture.requestBuffer();

	capture.startStreaming();

	for(int i=0; i<NUM_BUFFERS; i++)
	{        
        index = capture.readFrame();

		printf("capture frame %d and index %d size %d\n",i, index, size);

        encoder.encodeFrame(buffer[index], size);
        encoder1.encodeFrame(buffer[index], size);

		capture.queueBuffer(index);

	}

    encoder.stopStream();
    encoder1.stopStream();

	capture.stopStreaming();

    return NULL;
}


int main(int argc, char **argv)
{
    Paramters param1 = {"/dev/front_camera", "capture0", 1280, 720, 25, 2000000, 8000000, 30,3000,"192.0.77.99", true};
    Paramters param2 = {"/dev/bottom_camera", "capture1", 320,  240, 30, 2000000, 4000000, 90,4000,"192.0.77.99", false};

    std::thread thread1(mainloop, &param1);

    std::thread thread2(&mainloop, &param2);

    // wait for threads to finish
    thread1.join();
    thread2.join();

}
