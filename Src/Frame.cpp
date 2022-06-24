#include "Frame.hpp"

char Frame::GetFrameType(char *buffer){
    return buffer[0];    
}

int DataFrame::WriteToBuffer(char* buffer){
    buffer[0] = DATA;
    memcpy(buffer + 1, &seqNum, 4);
    memcpy(buffer + 5, &dest, 4);
    memcpy(buffer + 9, &src, 4);
    memcpy(buffer + 13, &flag, 1);
    memcpy(buffer + 14, &dataSize, 4);
    memcpy(buffer + 18, data, dataSize);
    return dataSize + 18;
}

int AckFrame::WriteToBuffer(char* buffer){
    buffer[0] = ACK;
    memcpy(buffer + 1, &seqNum, 4);
    memcpy(buffer + 5, &dest, 4);
    memcpy(buffer + 9, &src, 4);
    return 13;
}

AckFrame* AckFrame::GetFrame(char* buffer){
    AckFrame* frame = new AckFrame();
    memcpy(&frame->seqNum, buffer + 1, 4);
    memcpy(&frame->dest, buffer + 5, 4);
    memcpy(&frame->src, buffer + 9, 4);
    return frame;
}

DataFrame* DataFrame::GetFrame(char* buffer){
    DataFrame* frame = new DataFrame();
    memcpy(&frame->seqNum, buffer + 1, 4);
    memcpy(&frame->dest, buffer + 5, 4);
    memcpy(&frame->src, buffer + 9, 4);
    memcpy(&frame->flag, buffer + 13, 1);
    memcpy(&frame->dataSize, buffer + 14, 4);
    memcpy(frame->data, buffer + 18, frame->dataSize);
    return frame;

}