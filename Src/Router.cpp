#include "Frame.hpp"
#include <string>
#include <iostream>
#include <queue>
#include <thread>
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
    int bufferSize;

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

    void PushBuffer(Frame frame){
        bufferMutex.lock();
        if(bufferSize != MAX_BUFFER_SIZE){
            buffer[bufferTail] = frame;
            bufferTail = (bufferTail + 1) % MAX_BUFFER_SIZE;
            bufferSize++;
        }
        bufferMutex.unlock();
    }

    Frame PopBuffer(){
        Frame frame;
        bufferMutex.lock();
        if(bufferSize != MAX_BUFFER_SIZE){
            frame = buffer[bufferHead];
            bufferHead = (bufferHead + 1) % MAX_BUFFER_SIZE;
            bufferSize--;
        }
        bufferMutex.unlock();
        return frame;
    }

    void HandleInput() {
        sockaddr_in hostAddr;
        socklen_t addrSize;
        while(true) {
            recvfrom(routerFd, (char *)inputBuffer, MAX_FRAME_SIZE, MGS_WAITALL, (struct sockaddr *)&hostAddr, &addrSize);
            char frameType = Frame::GetFrameType(inputBuffer);
            Frame frame;
            if (frameType == DATA) {
                frame = DataFrame::GetFrame(inputBuffer);
            }
            else {
                frame = AckFrame::GetFrame(inputBuffer);
            }
            PushBuffer(frame);
        }
    }

    void SendData(){
        while (true){
            if(bufferSize == 0)
                continue;
            Frame frame = PopBuffer();
            int frameSize = frame.WriteToBuffer(outputBuffer);
            struct sockaddr_in hostAddr;
            hostAddr.sin_family = AF_INET;
            hostAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
            hostAddr.sin_port = htons(frame.dest);
            sendto(routerFd, outputBuffer, frameSize, 0, (struct sockaddr *) hostAddr, sizeof(hostAddr));
        }
    }



public:
    Router(int _port) {
        port = _port;
        bufferHead = 0;
        bufferTail = 0;
        bufferSize = 0;
    }
    void Run() {
        InitSocket();
        thread inputThread(HandleInput);
        SendData();
    }
};

int main() {
    Router router(ROUTER_PORT);
    router.Run();
    return 0;
}