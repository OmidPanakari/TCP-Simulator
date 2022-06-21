#ifndef UTILITY_HPP
#define UTILITY_HPP

#include <string>

#define MAX_DATA_SIZE 1500
#define MAX_FRAME_SIZE MAX_DATA_SIZE + 9

#define CONNECT 0
#define SEND 1
#define RECIEVE 2

struct DataFrame {
    int dataSize, seqNum, src, dest;
    char data[MAX_DATA_SIZE];
};

struct AckFrame {
    int seqNum, src, dest;
};

class Utility {
public:
    static char GetFrameType(char *buffer);
    static DataFrame ReadDataFrame(char *buffer);
    static AckFrame ReadAckFrame(char *buffer);

    static void WriteDataFrame(char *buffer, DataFrame frame);
    static void WriteAckFrame(char *buffer, AckFrame frame);
    static void WriteConnectFrame(char *buffer);
};

#endif