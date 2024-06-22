#include "encoder.hpp"
#include "capture.hpp"
#include "utils.hpp"

#include <string>
#include <thread>

#define NUM_BUFFERS 300

// The function will wait 3sec and then change the bitrate.
void task1(void* param)
{
    Encoder* enc = (Encoder*)param;
    std::this_thread::sleep_for(std::chrono::seconds(3));
    printf("\nChanges bitrate to 1mB \n");
    enc->setEncoderOption(V4L2_CID_MPEG_VIDEO_BITRATE, 1000000);
}

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

    encoder.init(videoParams->width, videoParams->height, videoParams->highBitrate, videoParams->fps, true, false, false);
    encoder1.init(videoParams->width, videoParams->height, videoParams->lowBitrate, videoParams->fps, false, false, false);

    std::thread t1(task1, &encoder);

	capture.queryCapabilites();

	capture.setFormat(videoParams->width, videoParams->height, V4L2_PIX_FMT_NV12);

    //capture.rotateImage(90);

	size = capture.requestBuffer();

	capture.startStreaming();

	for(int i=0; i<NUM_BUFFERS; i++)
	{        
        index = capture.readFrame();

		printf("capture frame %d and index %d size %d \n",i, index, size);

        encoder.encodeFrame(buffer[index], size);
        encoder1.encodeFrame(buffer[index], size);

		capture.queueBuffer(index);

	}

    t1.join();

    encoder.stopStream();
    encoder1.stopStream();

	capture.stopStreaming();

    return NULL;
}


int main(int argc, char **argv)
{
    Paramters param1 = {"/dev/front_camera", "capture0", 1280,  720, 25, 1000000, 2000000, 30,3000,  ADDRESS,  true};
    Paramters param2 = {"/dev/bottom_camera", "capture1", 320,  240, 30, 400000, 700000, 90,4000, ADDRESS, false};

    std::thread thread1(mainloop, &param1);

    std::thread thread2(&mainloop, &param2);

    // wait for threads to finish
    thread1.join();
    thread2.join();

}
