#ifndef UTILITY_HPP
#define UTILITY_HPP

#include <string>

#define MAX_DATA_SIZE 1500
#define MAX_FRAME_SIZE MAX_DATA_SIZE + 9

#define CONNECT 0
#define SEND 1
#define ACK 2

struct Frame {
public:
    virtual void WriteToBuffer(char *buffer) = 0;
    static char GetFrameType(char *buffer);
};

struct DataFrame: Frame {
    int dataSize, seqNum, src, dest;
    char data[MAX_DATA_SIZE];

public:
    void WriteToBuffer(char *buffer);
    static DataFrame GetFrame(char *buffer);
};

struct AckFrame: Frame {
    int seqNum, src, dest;

public:
    void WriteToBuffer(char *buffer);
    static AckFrame GetFrame(char *buffer);
};

struct ConnectFrame: Frame {
public:
    void WriteToBuffer(char *buffer);
    static ConnectFrame GetFrame(char *buffer);
};

#endif