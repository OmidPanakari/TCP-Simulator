#include "Frame.hpp"

char Frame::GetFrameType(char *buffer){
    return buffer[0];    
}

int DataFrame::WriteToBuffer(char* buffer){
    buffer[0] = DATA;
    memcpy(buffer + 1, &seqNum, 4);
    memcpy(buffer + 5, &dest, 4);
    memcpy(buffer + 9, &src, 4);
    memcpy(buffer + 13, &dataSize, 4);
    memcpy(buffer + 17, data, dataSize);
    return dataSize + 17;
}

int AckFrame::WriteToBuffer(char* buffer){
    buffer[0] = ACK;
    memcpy(buffer + 1, &seqNum, 4);
    memcpy(buffer + 5, &dest, 4);
    memcpy(buffer + 9, &src, 4);
    return 13;
}

AckFrame AckFrame::GetFrame(char* buffer){
    AckFrame frame;
    memcpy(&frame.seqNum, buffer + 1, 4);
    memcpy(&frame.dest, buffer + 5, 4);
    memcpy(&frame.src, buffer + 9, 4);
    return frame;
}

DataFrame DataFrame::GetFrame(char* buffer){
    DataFrame frame;
    memcpy(&frame.seqNum, buffer + 1, 4);
    memcpy(&frame.dest, buffer + 5, 4);
    memcpy(&frame.src, buffer + 9, 4);
    memcpy(&frame.dataSize, buffer + 13, 4);
    memcpy(frame.data, buffer + 17, frame.dataSize);
    return frame;

}