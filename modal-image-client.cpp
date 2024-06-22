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
#include <getopt.h>
#include <unistd.h>	// for usleep()
#include <string.h>
#include "utils.hpp"

#include <modal_start_stop.h>
#include <modal_pipe_client.h>


// this is the directory used by the voxl-hello-server for named pipes
#define PIPE_NAME	"capture0_high"

// you may need a larger buffer for your application!
#define PIPE_READ_BUF_SIZE	1024
#define CLIENT_NAME	"modal-hello-client"

static int en_debug;
static int en_auto_reconnect = 0;	// set with --auto-reconnect arg

NET_STAT net;



// called whenever the simple helper has data for us to process
static void my_client_camera_cb(int ch, camera_image_metadata_t meta,  char* frame,  void* context)
{
	NET_STAT* net = (NET_STAT*)context;

	printf("received %d bytes on channel %d: %d\n", meta.size_bytes , ch, meta.width);
	udpSend(net->ch, net->addr, frame, meta.size_bytes);

	return;
}

// called whenever we connect or reconnect to the server
static void _connect_cb(int ch, __attribute__((unused)) void* context)
{
	int ret;
	fprintf(stderr, "channel %d connected to server\n", ch);

	// send a hello message back to server via the control pipe (for fun)
	// not all servers will have a control pie, it's optional
	printf("sending hello to server control pipe\n");
	ret = pipe_client_send_control_cmd(0, "hello from client!");
	if(ret<0){
		fprintf(stderr, "failed to send control command to server\n");
		pipe_print_error(ret);
	}

	// now we are connected and before we read data,
	// check that the type is correct!!!
	// if(!pipe_is_type(PIPE_NAME, "text")){
	// 	fprintf(stderr, "ERROR, pipe is not of type \"text\"\n");
	// 	main_running = 0;
	// 	return;
	// }

	return;
}


// called whenever we disconnect from the server
static void _disconnect_cb(int ch, __attribute__((unused)) void* context)
{
	fprintf(stderr, "channel %d disconnected from server\n", ch);
	return;
}

int main(int argc, char* argv[])
{
	int ret;

	// set some basic signal handling for safe shutdown.
	// quitting without cleanup up the pipe can result in the pipe staying
	// open and overflowing, so always cleanup properly!!!
	enable_signal_handler();
	main_running = 1;

	// for this test we will use the simple helper with optional debug mode
	int flags 						= CLIENT_FLAG_EN_CAMERA_HELPER | CLIENT_FLAG_DISABLE_AUTO_RECONNECT;
	// if(en_debug)			flags  |= CLIENT_FLAG_EN_DEBUG_PRINTS;

	// // enable auto reconnect flag if requested
	// if(!en_auto_reconnect){
	// 	flags  |= CLIENT_FLAG_DISABLE_AUTO_RECONNECT;
	// }
	// else{
	// 	// in auto-reconnect mode, tell the user we are waiting
	// 	printf("waiting for server\n");
	// }
    net.ch = openSocket();
	std::string host = std::string("192.0.77.99");
    initAddr(net.addr, 5555, host);

	// assign callabcks for data, connection, and disconnect. the "NULL" arg
	// here can be an optional void* context pointer passed back to the callbacks
	pipe_client_set_camera_helper_cb(0, my_client_camera_cb, &net);
	pipe_client_set_connect_cb(0, _connect_cb, NULL);
	pipe_client_set_disconnect_cb(0, _disconnect_cb, NULL);


	// init connection to server. In auto-reconnect mode this will "succeed"
	// even if the server is offline, but it will connect later on automatically
    ret = pipe_client_open(0, PIPE_NAME, CLIENT_NAME, flags, 0);

	// check for success
	if(ret){
		fprintf(stderr, "ERROR opening channel:\n");
		pipe_print_error(ret);
		if(ret==PIPE_ERROR_SERVER_NOT_AVAILABLE){
			fprintf(stderr, "make sure to start modal-hello-server first!\n");
		}
		return -1;
	}

	// keep going until signal handler sets the main_running flag to 0
	while(main_running){
		usleep(500000);
	}

	// all done, signal pipe read threads to stop
	printf("closing\n");
	fflush(stdout);
	pipe_client_close_all();

	return 0;
}
