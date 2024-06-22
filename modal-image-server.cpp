/*******************************************************************************
 * Copyright 2022 ModalAI Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 ******************************************************************************/


#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <pthread.h>
#include <getopt.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

 #include <string>

#include <modal_start_stop.h>
#include <modal_pipe_server.h>
#include "utils.hpp"
#include "encoder.hpp"
#include "capture.hpp"

#define SERVER_NAME	"modal-hello-server"
#define HELLO_PIPE_NAME	"hello"
#define HELLO_PIPE_LOCATION (MODAL_PIPE_DEFAULT_BASE_DIR HELLO_PIPE_NAME "/")
#define CH			0 // arbitrarily use channel 0 for our one pipe

#define NUM_BUF_TO_RUN 2000

// vars
static int en_debug;
static double frequency_hz = 2.0;

static void control_pipe_handler(int ch, char* string, int bytes, __attribute__((unused)) void* context)
{
	printf("received command on channel %d bytes: %d string: \"%s\"\n", ch, bytes, string);
	return;
}


static void connect_handler(int ch, int client_id, char* client_name, __attribute__((unused)) void* context)
{
	printf("client \"%s\" connected to channel %d  with client id %d\n", client_name, ch, client_id);
	return;
}


static void disconnect_handler(int ch, int client_id, char* name, __attribute__((unused)) void* context)
{
	printf("client \"%s\" with id %d has disconnected from channel %d\n", name, client_id, ch);
	return;
}


int main(int argc, char* argv[])
{
////////////////////////////////////////////////////////////////////////////////
// gracefully handle an existing instance of the process and associated PID file
////////////////////////////////////////////////////////////////////////////////

	// make sure another instance isn't running
	// if return value is -3 then a background process is running with
	// higher privaledges and we couldn't kill it, in which case we should
	// not continue or there may be hardware conflicts. If it returned -4
	// then there was an invalid argument that needs to be fixed.
	if(kill_existing_process(SERVER_NAME, 2.0)<-2) return -1;

	// start signal handler so we can exit cleanly
	if(enable_signal_handler()==-1){
		fprintf(stderr,"ERROR: failed to start signal handler\n");
		return -1;
	}
	en_debug=1;

	Paramters param1 = {"/dev/front_camera", "capture0", 1280,  720, 25, 1000000, 2000000, 30,3000,  ADDRESS,  false};
	Paramters* videoParams = &param1;
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
    // Encoder encoder1(lowName, videoParams->basePort+1, videoParams->address);
    
    Capture capture(videoParams->devicePath, videoParams->outputFileName);

    capture.attachBuffers(buffer);
    encoder.init(videoParams->width, videoParams->height, videoParams->highBitrate, videoParams->fps, false, false, true, false);
    // encoder1.init(videoParams->width, videoParams->height, videoParams->lowBitrate, videoParams->fps, false, false, false);

	capture.queryCapabilites();

	capture.setFormat(videoParams->width, videoParams->height, V4L2_PIX_FMT_NV12);

    //capture.rotateImage(90);

	size = capture.requestBuffer();

	capture.startStreaming();

	for(int i=0; i<NUM_BUF_TO_RUN ; i++)
	{        
        index = capture.readFrame();

		printf("capture frame %d and index %d size %d \n",i, index, size);

        encoder.encodeFrame(buffer[index], size);
        // encoder1.encodeFrame(buffer[index], size);

		capture.queueBuffer(index);

	}

    encoder.stopStream();
    // encoder1.stopStream();

	capture.stopStreaming();

////////////////////////////////////////////////////////////////////////////////
// Stop all the threads and do cleanup HERE
////////////////////////////////////////////////////////////////////////////////

	printf("Starting shutdown sequence\n");
	pipe_server_close_all();
	remove_pid_file(SERVER_NAME);
	printf("exiting cleanly\n");
	return 0;
}
