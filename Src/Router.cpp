#include "Frame.hpp"
#include <string>
#include <iostream>
#include <queue>
#include <mutex>
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>

using namespace std;

#define ROUTER_PORT 5000
#define MAX_BUFFER_SIZE 1000


class Router {
    int port, routerFd, bufferHead, bufferTail;
    sockaddr_in routerAddr;
    char inputBuffer[MAX_FRAME_SIZE], outputBuffer[MAX_FRAME_SIZE];
    Frame buffer[MAX_BUFFER_SIZE];
    mutex bufferMutex;

    void InitSocket() {
        if ((routerFd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            cout << "Cannot create socket!" << endl;
            exit(0);
        }
        routerAddr.sin_family = AF_INET;
        routerAddr.sin_addr.s_addr = INADR_ANY;
        routerAddr.sin_port = htons(port);

        if (bind(routerFd, (const struct scokaddr *)&routerAddr, sizeof(routerAddr)) < 0) {
            cout << "Cannot bind socket to address" << endl;
            exit(0);
        }
    }

    void AddNewHost(sockaddr_in hostAddr) {
        
    }

    void HandleInput() {
        sockaddr_in hostAddr;
        socklen_t addrSize;
        while(true) {
            recvfrom(routerFd, (char *)inputBuffer, MAX_FRAME_SIZE, MGS_WAITALL, (struct sockaddr *)&hostAddr, &addrSize);
            char frameType = Frame::GetFrameType(inputBuffer);
            if (frameType == CONNECT) {
                AddNewHost(hostAddr);
            }
            else if (frameType == SEND) {

            }
            else {

            }
        }
    }



public:
    Router(int _port) {
        port = _port;
        bufferHead = 0;
        bufferTail = 0;
    }
    void Run() {
        InitSocket();
        
        
    }
};

int main() {
    Router router(ROUTER_PORT);
    router.Run();
    return 0;
}