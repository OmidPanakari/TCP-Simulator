#ifndef UTILITY_HPP
#define UTILITY_HPP
#include <string>
#define MAX_DATA_SIZE 1500
#define MAX_FRAME_SIZE MAX_DATA_SIZE + 9

#define DATA 0
#define ACK 1

#define FIN 1

struct Frame {
    int seqNum, src, dest;
    virtual int WriteToBuffer(char *buffer) = 0;
    static char GetFrameType(char *buffer);
};

struct DataFrame: Frame {
    char flag;
    int dataSize;
    char data[MAX_DATA_SIZE];
    int WriteToBuffer(char *buffer);
    static DataFrame GetFrame(char *buffer);
};

struct AckFrame: Frame {
    int WriteToBuffer(char *buffer);
    static AckFrame GetFrame(char *buffer);
};

#endif