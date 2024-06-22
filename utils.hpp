
#ifndef __UTILS_hpp__
#define __UTILS_hpp__

#include <string>

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>

#include <modal_pipe_server.h>


#define ADDRESS "192.0.65.99"


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

typedef struct {
	sockaddr_in addr;
    int ch;
}NET_STAT;



bool udpSend(int socket, const sockaddr_in& addr, void* msg, int size);

void initAddr(sockaddr_in& addr, int port, std::string& address);

int openSocket();

void closeSocket(int socket);

void fixBug();

#endif //__UTILS_hpp__