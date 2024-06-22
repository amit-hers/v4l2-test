#include "utils.hpp"


#define MAX_SIZE_UDP 65535


bool udpSend(int socket, const sockaddr_in& addr, void* msg, int size)
{
    if(size >= MAX_SIZE_UDP)
    {
        printf("cannot send udp to big %d", size);
        return false;
    }

    if (sendto(socket, (const char *) msg, size, 0, (sockaddr*)&addr, sizeof(addr)) < 0)
    {
        perror("cannot send message");
        close(socket);
        return false;
    }
    return true;
}

void initAddr(sockaddr_in& addr, int port, std::string& address)
{
    bzero(&addr,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(address.c_str());
    addr.sin_port = htons(port);
}

int openSocket()
{
    int socketFd= socket(AF_INET,SOCK_DGRAM,0);
    if(socketFd<0){
        perror("cannot open socket");
        return -1;
    }

    return socketFd;
}

void closeSocket(int socket)
{
    close(socket);
}

void fixBug()
{
    // Export the desired pin by writing to /sys/class/gpio/export

    int fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd == -1) {
        perror("Unable to open /sys/class/gpio/export");
        exit(1);
    }

    if (write(fd, "10", 2) != 2) {
        perror("Error writing to /sys/class/gpio/export");
        exit(1);
    }

    close(fd);

    // Set the pin to be an output by writing "out" to /sys/class/gpio/gpio10/direction

    fd = open("/sys/class/gpio/gpio10/direction", O_WRONLY);
    if (fd == -1) {
        perror("Unable to open /sys/class/gpio/gpio10/direction");
        exit(1);
    }

    if (write(fd, "out", 3) != 3) {
        perror("Error writing to /sys/class/gpio/gpio10/direction");
        exit(1);
    }

    close(fd);

    fd = open("/sys/class/gpio/gpio10/value", O_WRONLY);
    if (fd == -1) {
        perror("Unable to open /sys/class/gpio/gpio10/value");
        exit(1);
    }

    // Toggle LED 50 ms on, 50ms off, 100 times (10 seconds)

    if (write(fd, "1", 1) != 1) {
        perror("Error writing to /sys/class/gpio/gpio10/value");
        exit(1);
    }
    usleep(50000);

    if (write(fd, "0", 1) != 1) {
        perror("Error writing to /sys/class/gpio/gpio10/value");
        exit(1);
    }
    usleep(50000);
    

    close(fd);

    // Unexport the pin by writing to /sys/class/gpio/unexport

    fd = open("/sys/class/gpio/unexport", O_WRONLY);
    if (fd == -1) {
        perror("Unable to open /sys/class/gpio/unexport");
        exit(1);
    }

    if (write(fd, "10", 2) != 2) {
        perror("Error writing to /sys/class/gpio/unexport");
        exit(1);
    }

    close(fd);

}