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
#define MIN_THRESH_HOLD 100
#define MAX_THRESH_HOLD 900
#define RED_WEIGHT 0.002
#define MAX_P 0.1

const bool RED_ENABLE = false;
const bool DROP_ENABLE = true;


class Router {
    int port, routerFd, bufferHead, bufferTail;
    sockaddr_in routerAddr;
    char inputBuffer[MAX_FRAME_SIZE], outputBuffer[MAX_FRAME_SIZE];
    Frame* buffer[MAX_BUFFER_SIZE];
    mutex bufferMutex;
    int bufferSize;
    float meanBufferSize;
    int count;

    void InitSocket() {
        if ((routerFd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            cout << "Cannot create socket!" << endl;
            exit(0);
        }
        routerAddr.sin_family = AF_INET;
        routerAddr.sin_addr.s_addr = INADDR_ANY;
        routerAddr.sin_port = htons(port);

        if (bind(routerFd, (const struct sockaddr *)&routerAddr, sizeof(routerAddr)) < 0) {
            cout << "Cannot bind socket to address" << endl;
            exit(0);
        }
    }

    void PushBuffer(Frame* frame){
        bufferMutex.lock();
        if(bufferSize < MAX_BUFFER_SIZE){
            buffer[bufferTail] = frame;
            bufferTail = (bufferTail + 1) % MAX_BUFFER_SIZE;
            bufferSize++;
        }else {
            cout << "Dropping Packet"<< endl;
        }
        bufferMutex.unlock();
    }

    Frame* PopBuffer(){
        Frame* frame;
        bufferMutex.lock();
        if(bufferSize > 0){
            frame = buffer[bufferHead];
            bufferHead = (bufferHead + 1) % MAX_BUFFER_SIZE;
            bufferSize--;
            bufferMutex.unlock();
            return frame;
        }
        bufferMutex.unlock();
        return nullptr;
    }

    void UpdateMeanBufferSize(){
        meanBufferSize = (1 - RED_WEIGHT) * meanBufferSize + RED_WEIGHT * bufferSize;
    }

    void HandleInput() {
        sockaddr_in hostAddr;
        socklen_t addrSize;
        while(true) {
            recvfrom(routerFd, (char *)inputBuffer, MAX_FRAME_SIZE, MSG_WAITALL, (struct sockaddr *)&hostAddr, &addrSize);
            if (DROP_ENABLE){
                int i = rand() % 100;
                if (i < 10)
                {
                    cout << "Dropping Packet"<< endl;
                    continue;
                }
            }
            if (RED_ENABLE && meanBufferSize >= MIN_THRESH_HOLD)
            {
                if (meanBufferSize >= MAX_THRESH_HOLD){
                    count = 0;
                    UpdateMeanBufferSize();
                    continue;
                }else{
                    float tempP = MAX_P * (meanBufferSize - MIN_THRESH_HOLD) / (MAX_THRESH_HOLD - MIN_THRESH_HOLD);
                    float p = tempP / (1 - count * tempP);
                    srand(time(0));
                    int i = rand() % 100;
                    if (i < p * 100)
                    {
                        count = 0;
                        UpdateMeanBufferSize();
                        continue;
                    }
                }
            }
            UpdateMeanBufferSize();
            count++;
            char frameType = Frame::GetFrameType(inputBuffer);
            Frame* frame;
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
            Frame* frame = PopBuffer();
            int frameSize = frame->WriteToBuffer(outputBuffer);
            struct sockaddr_in hostAddr;
            hostAddr.sin_family = AF_INET;
            hostAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
            hostAddr.sin_port = htons(frame->dest);
            sendto(routerFd, outputBuffer, frameSize, 0, (struct sockaddr *) &hostAddr, sizeof(hostAddr));
        }
    }



public:
    Router(int _port) {
        port = _port;
        bufferHead = 0;
        bufferTail = 0;
        bufferSize = 0;
        meanBufferSize = 0;
        count = 0;
    }
    void Run() {
        InitSocket();
        thread inputThread(&Router::HandleInput, this);
        SendData();
    }
};

int main() {
    srand(time(0));
    Router router(ROUTER_PORT);
    router.Run();
    return 0;
}