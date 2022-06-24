#include "Frame.hpp"
#include <string>
#include <iostream>
#include <fstream>
#include <chrono>
#include <mutex>
#include <vector>
#include <map>
#include <string>
#include <thread>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h> 
#include <netinet/in.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>

using namespace std;

#define WINDOW_SIZE 20
#define MAX_SEQ (2 * WINDOW_SIZE)

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
    bool isFinished;
};

struct Sender:Window {
    int receiverPort, hostFd, fileSize;
    string fileName;
    ifstream file;
    chrono::time_point<chrono::high_resolution_clock> start;
};

struct Receiver:Window {
    int senderPort, hostFd;
    string file;
};

class Host {
    int hostPort, routerPort, hostFd;
    sockaddr_in hostAddr;
    Sender sender;
    map<int, Receiver> receivers;
    char inputBuffer[MAX_FRAME_SIZE], outputBuffer[MAX_FRAME_SIZE];
    mutex senderMutex, receiverMutex;

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
        hostAddr.sin_addr.s_addr = INADDR_ANY;
        hostAddr.sin_port = htons(hostPort);

        if (bind(hostFd, (const struct sockaddr *)&hostAddr, sizeof(hostAddr)) < 0) {
            cout << "Cannot bind socket to address" << endl;
            exit(0);
        }
    }

    bool ContainsSeqNum(Window window, int seqNum) {
        if (window.windowTail < window.windowHead) {
            return seqNum >= window.windowHead || seqNum < window.windowTail;
        }
        else {
            return seqNum >= window.windowHead && seqNum < window.windowTail;
        }
    }

    void ReceiveAck(AckFrame ack) {
        senderMutex.lock();
        if (ContainsSeqNum(sender, ack.seqNum)) {
            cout << "receiving ack : "<< ack.seqNum << endl;
            sender.window[ack.seqNum % WINDOW_SIZE].acknowledged = true;
        }
        senderMutex.unlock();
    }

    void SendAck(DataFrame data) {
        AckFrame ack;
        ack.src = hostPort;
        ack.dest = data.src;
        ack.seqNum = data.seqNum;
        char buffer[MAX_DATA_SIZE];
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        addr.sin_port = htons(routerPort);
        int ackSize = ack.WriteToBuffer(buffer);
        sendto(hostFd, buffer, ackSize, 0, (struct sockaddr *)&addr, sizeof(addr));
    }

    void FillPacket(Packet *packet, DataFrame data) {
        packet->content = (char *)malloc(data.dataSize);
        packet->dataSize = data.dataSize;
        memcpy(packet->content, data.data, data.dataSize);
        packet->acknowledged = true;
        packet->isLast = (data.flag == FIN);
    }

    void ReceiveData(DataFrame data) {
        auto it = receivers.find(data.src);
        if (it != receivers.end()) {
            Receiver *receiver = &it->second;
            receiverMutex.lock();
            if (!receiver->isFinished && ContainsSeqNum(*receiver, data.seqNum)) {
                FillPacket(&receiver->window[data.seqNum % WINDOW_SIZE], data);
                receiverMutex.unlock();
            }
            else {
                receiverMutex.unlock();
                SendAck(data);
            }
        }
        else {
            Receiver newReceiver;
            newReceiver.windowHead = 0;
            newReceiver.windowTail = 0;
            newReceiver.windowSize = 0;
            newReceiver.isFinished = false;
            string path = "Receiver/" + ToString(data.src) + ".txt";
            remove(path.c_str());
            newReceiver.file = "Receiver/" + ToString(data.src) + ".txt";
            while(newReceiver.windowSize < WINDOW_SIZE) {
                Packet newPacket;
                newPacket.acknowledged = false;
                newPacket.isLast = false;
                newReceiver.window[newReceiver.windowTail % WINDOW_SIZE] = newPacket;
                newReceiver.senderPort = data.src;
                newReceiver.windowSize++;
                newReceiver.windowTail = (newReceiver.windowTail + 1) % MAX_SEQ;
            }
            FillPacket(&newReceiver.window[data.seqNum % WINDOW_SIZE], data);
            receivers[data.src] = newReceiver;
        }
    }

    void ReceiveFrames() {
        sockaddr_in addr;
        socklen_t addrLen;
        while (true) {
            recvfrom(hostFd, (char *)inputBuffer, MAX_FRAME_SIZE, MSG_WAITALL, (struct sockaddr *)&addr, &addrLen);
            int frameType = Frame::GetFrameType(inputBuffer);
            if (frameType == DATA) {
                DataFrame data = *DataFrame::GetFrame(inputBuffer);
                cout << "receiving packet : " << data.seqNum << endl;
                ReceiveData(data);
            }
            else {
                AckFrame ack = *AckFrame::GetFrame(inputBuffer);
                ReceiveAck(ack);
            }
        }
    }

    void SendPacket(Packet packet, int receiverPort, int seqNum) {
        DataFrame data;
        data.src = hostPort;
        data.dest = receiverPort;
        data.flag = (packet.isLast) ? FIN : 0;
        data.dataSize = packet.dataSize;
        data.seqNum = seqNum;
        memcpy(data.data, packet.content, packet.dataSize);
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        addr.sin_port = htons(routerPort);
        int frameSize = data.WriteToBuffer(outputBuffer);
        cout<< "sending packet :" << seqNum <<endl;
        sendto(hostFd, outputBuffer, frameSize, 0, (struct sockaddr *)&addr, sizeof(addr));
    }

    void SendPacketAck(Packet packet, int receiverPort, int seqNum) {
        AckFrame ack;
        ack.src = hostPort;
        ack.dest = receiverPort;
        ack.seqNum = seqNum;
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        addr.sin_port = htons(routerPort);
        int frameSize = ack.WriteToBuffer(outputBuffer);
        cout << "sending ack : "<< ack.seqNum << endl;
        sendto(hostFd, outputBuffer, frameSize, 0, (struct sockaddr *)&addr, sizeof(addr));
    }

    void CheckSender() {
        senderMutex.lock();
        if (sender.isFinished) {
            senderMutex.unlock();
            return;
        }
        Packet packet;
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
                cout << chrono::duration_cast<chrono::milliseconds>(tm - sender.window[i% WINDOW_SIZE].start).count() << endl;
                sender.window[i% WINDOW_SIZE].start = tm;
                SendPacket(sender.window[i% WINDOW_SIZE], sender.receiverPort, i);
            }
        }

        while (sender.fileSize && sender.windowSize != WINDOW_SIZE) {
            int readSize = MAX_DATA_SIZE;
            if (sender.fileSize <= MAX_DATA_SIZE) {
                readSize = sender.fileSize;
                sender.fileSize = 0;
            }
            else {
                sender.fileSize -= MAX_DATA_SIZE;
            }
            packet.content = (char *)malloc(readSize);
            packet.dataSize = readSize;
            packet.acknowledged = false;
            packet.isLast = (sender.fileSize == 0);
            sender.windowSize++;
            sender.file.read((char *)packet.content, packet.dataSize);
            packet.start = chrono::high_resolution_clock::now();
            sender.window[sender.windowTail % WINDOW_SIZE] = packet;
            sender.windowTail = (sender.windowTail + 1) % MAX_SEQ;
            senderMutex.unlock();
            SendPacket(packet, sender.receiverPort, (sender.windowTail - 1 + MAX_SEQ) % MAX_SEQ);
            senderMutex.lock();
        }

        senderMutex.unlock();
    }

    void CheckReceivers() {
        vector<int> finished;
        map<int, Receiver>::iterator it;
        for (it = receivers.begin(); it != receivers.end(); it++) {
            Receiver *receiver = &(it->second);
            if (receiver->isFinished) continue;
            receiverMutex.lock();
            while(receiver->windowSize && receiver->window[receiver->windowHead % WINDOW_SIZE].acknowledged) {
                Packet *packet = &(receiver->window[receiver->windowHead % WINDOW_SIZE]);
                receiver->windowHead = (receiver->windowHead + 1) % MAX_SEQ;
                receiver->windowSize--;
                //receiverMutex.unlock();
                ofstream file(receiver->file, ios::app);
                file.write(packet->content, packet->dataSize);
                file.close();
                SendPacketAck(*packet, receiver->senderPort, (receiver->windowHead - 1 + MAX_SEQ) % MAX_SEQ);
                free(packet->content);
                if (packet->isLast) {
                    receiver->isFinished = true;
                    finished.push_back(it->first);
                }
                //receiverMutex.lock();
            }
            if (receiver->isFinished) continue;
            Packet packet;
            while(receiver->windowSize < WINDOW_SIZE) {
                packet.acknowledged = false;
                packet.isLast = false;
                receiver->window[receiver->windowTail % WINDOW_SIZE] = packet;
                receiver->windowSize++;
                receiver->windowTail = (receiver->windowTail + 1) % MAX_SEQ;
            }
            receiverMutex.unlock();
        }
//        for (int ind: finished) {
//            receivers.erase(receivers.find(ind));
//        }
    }

    void Check() {
        while(true) {
            CheckSender();
            CheckReceivers();
        }
    }

    void StartSend(int receiverPort) {
        senderMutex.lock();
        sender.file.open("Sender/" + ToString(hostPort) + ".txt", ios::in);
        sender.file.seekg(0, sender.file.end);
        sender.fileSize = sender.file.tellg();
        sender.file.seekg(0, sender.file.beg);
        sender.windowHead = 0;
        sender.windowTail = 0;
        sender.windowSize = 0;
        sender.receiverPort = receiverPort;
        sender.isFinished = false;
        sender.start = chrono::high_resolution_clock::now();
        senderMutex.unlock();
    }

public:
    Host(int _hostPort, int _routerPort) {
        hostPort = _hostPort;
        routerPort = _routerPort;
    }

    void Run() {
        InitSocket();
        sender.fileSize = 0;
        sender.windowSize = 0;
        sender.isFinished = true;
        thread receiveThread(&Host::ReceiveFrames, this);
        thread checkThread(&Host::Check, this);
        string cmd;
        while(true) {
            cin >> cmd;
            if (cmd == "send") {
                int receiverPort;
                cin >> receiverPort;
                StartSend(receiverPort);
            }
        }
    }
};

int main(int argc, char *argv[]) {
    if (argc < 2) {
        cout << "Not enough arguments!" << endl;
        return 0;
    }

    Host host(atoi(argv[1]), ROUTER_PORT);
    host.Run();
    return 0;
}