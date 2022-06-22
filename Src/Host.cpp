#include "Frame.hpp"
#include <string>
#include <iostream>
#include <dequeue>
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>

using namespace std;

#define WINDOW_SIZE 20
#define MAX_SEQ 2 * WINDOW_SIZE

struct Packet {
    bool acknowledged;
    char *content;
    int seqNum, dataSize;
};

int main(int argc, char *argv[]) {
    if (argc < 2) {
        cout << "Not enough arguments!" << endl;
        return 0;
    }

    return 0;
}