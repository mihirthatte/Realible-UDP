#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <unistd.h>
#include <ctime>
#include <chrono>

#define DATASIZE 1460
#define BUFFSIZE 1472
#define RECVWINDOW 65535
#define MAXFILESIZE 10000000
#define TIMEOUT_MS 100000

using namespace std;

unsigned int baseNumber = 0;
bool congestion_avoidance;
bool fast_recovery;
bool slow_start;

int congestion_avoidance_count;
int slow_start_count;
int fast_recovery_count;
int count_of_packets;

int deviation = 1;
int threshold = 0;
int cwnd = 1;
int duplicate_count = 0;
long long estimated_RTT = 0;

#ifndef FUNCTIONS_H_INCLUDED
#define FUNCTIONS_H_INCLUDED
void readFile(char* file_data, string file_name, int& file_size);
#endif
