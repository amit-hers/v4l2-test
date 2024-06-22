
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



bool udpSend(int socket, const sockaddr_in& addr, void* msg, int size);

void initAddr(sockaddr_in& addr, int port, std::string& address);

int openSocket();

void closeSocket(int socket);

void fixBug();

#endif //__UTILS_hpp__