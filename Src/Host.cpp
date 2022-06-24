#include "Frame.hpp"
#include <string>
#include <iostream>
#include <fstream>
#include <chrono>
#include <mutex>
#include <map>
#include <string>
#include <thread>
#include <sstream>
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>

using namespace std;

#define WINDOW_SIZE 20
#define MAX_SEQ 2 * WINDOW_SIZE

#define TIMEOUT 10

#define ROUTER_PORT 5000

struct Packet {
    bool acknowledged, isLast;
    char *content;
    int dataSize;
    chrono::time_point<chrono::high_resolution_clock> start;
};

struct Window {
    Packet window[WINDOW_SIZE];
    int windowHead, windowTail, windowSize;
    mutex windowMutex;
    bool isFinished;
}

struct Sender:Window {
    int recieverPort, hostFd, fileSize;
    string fileName;
    ifstream file;
    chrono::time_point<chrono::high_resolution_clock> start;
};

struct Reciever:Window {
    int senderPort, hostFd;
    ofstream file;
};

class Host {
    int hostPort, routerPort, hostFd;
    sockaddr_in hostAddr;
    Sender sender;
    map<int, Reciever> recievers;
    char inputBuffer[MAX_FRAME_SIZE], outputBuffer[MAX_FRAME_SIZE];

    string ToString(int a) {
        stringstream ss;
        ss << a;
        return ss.str();
    }

    void InitSocket() {
        if ((hostFd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            cout << "Cannot create socket!" << endl;
            exit(0);
        }
        hostAddr.sin_family = AF_INET;
        hostAddr.sin_addr.s_addr = INADR_ANY;
        hostAddr.sin_port = htons(hostPort);

        if (bind(hostFd, (const struct scokaddr *)&hostAddr, sizeof(hostAddr)) < 0) {
            cout << "Cannot bind socket to address" << endl;
            exit(0);
        }
    }

    bool ContainsSeqNum(Window window, int seqNum) {
        if (window.windowTail < window.windowHead) {
            return seqNum >= window.windowHead || seqNum < window.windowTail;
        }
        else {
            return seqNum >= window.windowHead && seqNum < window.windowTail
        }
    }

    void RecieveAck(AckFrame ack) {
        sender.windowMutex.lock();
        if (ContainsSeqNum(sender, ack.seqNum)) {
            sender.window[ack.seqNum % WINDOW_SIZE].acknowledged = true;
        }
        sender.windowMutex.unlock();
    }

    void SendAck(DataFrame data) {
        AckFrame ack;
        ack.src = hostPort;
        ack.dest = data.src;
        ack.seqNum = data.seqNum;
        char buffer[MAX_BUFFER_SIZE];
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        addr.sin_port = htons(routerPort);
        int ackSize = ack.WriteToBuffer(buffer);
        sendto(hostFd, buffer, ackSize, 0, (struct sockaddr *)addr, sizeof(addr));
    }

    void FillPacket(Packet *packet, DataFrame data) {
        paket->content = (char *)malloc(data.dataSize);
        packet->dataSize = data.dataSize;
        memcpy(packet->content, data.data, data.dataSize);
        packet->acknowledged = true;
        packet->isLast = (data.flag == FIN);
    }

    void RecieveData(DataFrame data) {
        auto it = recievers.find(data.src);
        if (it != recievers.end()) {
            Reciever *reciever = &it->second;
            reciever->windowMutex.lock();
            if (ContainsSeqNum(*reciever, data.seqNum)) {
                FillPacket(&reciever->window[data.seqNum % WINDOW_SIZE], data);
                reciever->windowMutex.unlock();
            }
            else {
                reciever->windowMutex.unlock();
                SendAck(data);
            }
        }
        else {
            Reciever newReciever;
            newReciever.windowHead = 0;
            newReciever.windoTail = 0;
            newReciever.file.open("Reciever/" + ToString(data.src) + ".txt", ios::out);
            while(newReciever.windowSize < WINDOW_SIZE) {
                Packet newPacket;
                newPacket.acknowledged = false;
                newReciever.window[newReciever.windoTail%WINDOW_SIZE] = newPacket;
                newReciever.windowSize++;
                newReciever.windowTail = (newReciever.windowTail + 1) % MAX_SEQ;
            }
            FillPacket(&newReciever.window[data.seqNum % WINDOW_SIZE], data);
            recievers[data.src] = newReciever;
        }
    }

    void RecieveFrames() {
        sockaddr_in addr;
        socklen_t addrLen;
        while (true) {
            recvfrom(hostFd, (char *)inputBuffer, MAX_FRAME_SIZE, MGS_WAITALL, (struct sockaddr *)&addr, &addrLen);
            int frameType = Frame::GetFrameType(inputBuffer);
            if (frameType == DATA) {
                DataFrame data = DataFrame::GetFrame(inputBuffer);
                RecieveData(data);
            }
            else {
                AckFrame ack = AckFrame::GetFrame(inputBuffer);
                RecieveAck(ack);
            }
        }
    }

    void SendPacket(Packet packet, int recieverPort, int seqNum) {
        DataFrame data;
        data.src = hostPort;
        data.dest = recieverPort;
        data.flag = (packet.isLast) ? FIN : 0;
        data.dataSize = packet.dataSize;
        data.seqNum = seqNum;
        memcpy(data.data, packet.content, packet.dataSize);
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr("127.0.0.1"););
        addr.sin_port = htons(data.dest);
        int frameSize = data.WriteToBuffer(outputBuffer);
        sendto(hostFd, outputBuffer, frameSize, 0, (struct sockaddr *)addr, sizeof(addr));
    }

    void SendPacketAck(Packet packet, int recieverPort, int seqNum) {
        AckFrame ack;
        ack.src = hostPort;
        ack.dest = recieverPort;
        ack.seqNum = seqNum;
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr("127.0.0.1"););
        addr.sin_port = htons(ack.dest);
        int frameSize = ack.WriteToBuffer(outputBuffer);
        sendto(hostFd, outputBuffer, frameSize, 0, (struct sockaddr *)addr, sizeof(addr));
    }

    void CheckSender() {
        if (sneder.isFinished) {
            continue;
        }
        Packet packet;
        sender.windowMutex.lock();
        while (sender.windowSize && sender.window[sender.windowHead % WINDOW_SIZE].acknowledged) {
            sender.windowSize--;
            packet = sender.window[sender.windowHead % WINDOW_SIZE];
            sender.windowHead = (sender.windowHead + 1) % MAX_SEQ;
            free(packet.content);
            if (packet.isLast) {
                sender.isFinished = true;
                return;
            }
        }
        for (int i = sender.windowHead; i != sender.windowTail; i = (i+1)% MAX_SEQ) {
            auto tm = chrono::high_resolution_clock::now();
            if (chrono::duration_cast<chrono::milliseconds>(tm - sender.window[i% WINDOW_SIZE].start).count() >= TIMEOUT) {
                sender.window[i% WINDOW_SIZE].start = tm;
                SendPacket(sender.window[i% WINDOW_SIZE], sender.recieverPort, i);
            }
        }
        while (sender.fileSize && sender.windowSize != WINDOW_SIZE) {
            int readSize = MAX_BUFFER_SIZE;
            if (sender.fileSize <= MAX_BUFFER_SIZE) {
                readSize = sender.fileSize;
                sender.fileSize = 0;
            }
            else {
                sender.fileSize -= MAX_BUFFER_SIZE;
            }
            packet.content = (char *)malloc(readSize);
            packet.dataSize = readSize;
            packet.acknowledged = false;
            packet.isLast = false;
            packet.isLast = (sender.fileSize == 0);
            sender.file.read((char *)packet.content, packet.dataSize);
            sender.window[sender.windowTail % WINDOW_SIZE] = packet;
            sender.windowTail = (sender.windowTail + 1) % MAX_SEQ;
            sender.windowMutex.unlock();
            packet.start = chrono::high_resolution_clock::now();
            SendPacket(packet, sender.recieverPort, (sender.windowTail - 1 + MAX_SEQ) % MAX_SEQ);
            sender.windowMutex.lock();
        }
        sender.windowMutex.unlock();
    }

    void CheckRecievers() {
        vector<int> finished;
        for (auto it : recievers) {
            Reciever *reciever = &(it->second);
            reciever->windowMutex.lock();
            while(reciever->windowSize && reciever->window[reciever.windowHead % WINDOW_SIZE].acknowledged) {
                Packet *packet = &(reciever->window[reciever.windowHead % WINDOW_SIZE]);
                reciever->windowHead = (reciever->windowHead + 1) % MAX_SEQ;
                reciever->windowSize--;
                reciever->windowMutex.unlock();
                reciever->file.write(packet->content, packet->dataSize);
                SendPacketAck(&packet);
                free(packet->content);
                if (packet->isLast) {
                    reciever->file.close();
                    reciever->isFinished = true;
                    finished.push_back(it->first);
                }
                reciever->windowMutex.lock();
            }
            if (reciever->isFinished) continue;
            Packet packet;
            while(reciever->windowSize < WINDOW_SIZE) {
                packet.acknowledged = false;
                reciever->window[reciever->windowTail % WINDOW_SIZE] = packet;
                reciever->windowSize++;
                reciever->windowTaile = (reciever->windowTail + 1) % MAX_SEQ;
            }
            reciever->windowMutex.unlock();
        }
        for (int ind: finished) {
            recievers.erase(recievers.find(ind));
        }
    }

    void Check() {
        while(true) {
            CheckSender();
            CheckRecievers();
        }
    }

    void StartSend(int recieverPort) {
        sender.file.open("Sender/" + ToString(hostPort) + ".txt", ios::in);
        sender.file.seekg(0, sender.file.end);
        sender.fileSize = sender.file.tellg();
        sender.file.seekg(0, sender.file.beg);
        sender.recieverPort = recieverPort;
        sender.isFinished = false;
        sender.start = chrono::high_resolution_clock::now();
    }

public:
    Host(int _hostPort, _routerPort) {
        hostPort = _hostPort;
        routerPort = _routerPort;
    }

    void Run() {
        InitSocket();
        sender.fileSize = 0;
        sender.windowSize = 0;
        sender.isFinished = true;
        thread recieveThread(RecieveFrames);
        thread checkThread(Check);
        string cmd;
        while(true) {
            cin >> cmd;
            if (cmd == "send") {
                int recieverPort;
                cin >> recieverPort;
                StartSend(recieverPort);
            }
        }
    }
};

int main(int argc, char *argv[]) {
    if (argc < 2) {
        cout << "Not enough arguments!" << endl;
        return 0;
    }

    Host host(atoi(argv[1], ROUTER_PORT));
    Host.Run();
    return 0;
}