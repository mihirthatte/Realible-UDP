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

bool congestion_avoidance = false;
bool fast_recovery = false;
bool slow_start = true;

int congestion_avoidance_count = 0;
int slow_start_count = 0;
int fast_recovery_count = 0;
int count_of_packets;

int deviation = 1;
int threshold = 0;
int cwnd = 1;
int duplicate_count = 0;
long long estimated_RTT = 0;
long long calculate_timeout(long long sample_RTT){
    sample_RTT -= estimated_RTT>>3;
    estimated_RTT += sample_RTT;
    if (sample_RTT < 0 )
        sample_RTT = - 1 * sample_RTT;
    sample_RTT -= deviation >> 3;
    deviation += sample_RTT;
    //cout<<"Estimated time "<<estimated_RTT<<endl;
    return ((estimated_RTT >> 3) + (deviation >> 1));
}


void parseAcknowledgement(char* buffer, unsigned int& sequenceNumber, unsigned int& acknowledgementNumber, int& byte_index, int prev_bytes){
  unsigned char bytes[4];
  int iterator = 0;
  bytes[0] = buffer[iterator++];
  bytes[1] = buffer[iterator++];
  bytes[2] = buffer[iterator++];
  bytes[3] = buffer[iterator++];
  unsigned int seqNum = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | (bytes[3]);
  //cout<<sequenceNumber<<endl;
  bytes[0] = buffer[iterator++];
  bytes[1] = buffer[iterator++];
  bytes[2] = buffer[iterator++];
  bytes[3] = buffer[iterator++];

  unsigned int ackNum = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | (bytes[3]);
  //cout<<acknowledgementNumber<<endl;
  bytes[0] = buffer[iterator++];
  bytes[1] = buffer[iterator++];
  bytes[2] = buffer[iterator++];
  bytes[3] = buffer[iterator++];

  unsigned int recvWin = (bytes[0] << 8) | (bytes[1]);
  //cout<<receiveWindow<<endl;
  if((bytes[3] & 1) == 0){
    cout<<"Acknowledgement flag is not set"<<endl;
    return;
  }
  if(acknowledgementNumber != seqNum){
    cout<<"Server acknowledgement number did not match with the client sequence number."<<endl;
    return;
  }


if(sequenceNumber == ackNum){
  duplicate_count++;
  if(duplicate_count == 3){
    threshold = cwnd/2;
    cwnd = threshold + 3;
  }
  if(duplicate_count > 3){
    cwnd+=1;
  }
  return;
}

if(cwnd * 2 > threshold){
  if(!congestion_avoidance){
    if(slow_start){
        cout<<endl;
        cout<<"Changing from Slow start to Congestion Avoidance."<<endl;
        cout<<endl;
    }
    else{
      cout<<endl;
      cout<<"Changing from fast Recovery to Congestion Avoidance."<<endl;
      cout<<endl;
    }
    fast_recovery = false;
    slow_start = false;
    congestion_avoidance = true;
  }
  cwnd+=1;
}
else{
  cwnd*=2;
}
if(duplicate_count >= 3){
  slow_start = false;
  congestion_avoidance = false;
  if(!fast_recovery){
    cout<<endl;
    cout<<"3 duplicate ACK received. Changing the state from Congestion Avoidance to Fast Recovery"<<endl;
    cout<<endl;
    fast_recovery = true;
  }
  cwnd = threshold;
}
  duplicate_count = 0;
 sequenceNumber = ackNum;

  byte_index = ackNum;
}

string parseRequest(char* buffer, unsigned int& acknowledgementNumber){
  unsigned char bytes[4];
  int iterator = 0;

  bytes[0] = buffer[iterator++];
  bytes[1] = buffer[iterator++];
  bytes[2] = buffer[iterator++];
  bytes[3] = buffer[iterator++];
  unsigned int seqNum = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | (bytes[3]);
  //cout<<sequenceNumber<<endl;
  bytes[0] = buffer[iterator++];
  bytes[1] = buffer[iterator++];
  bytes[2] = buffer[iterator++];
  bytes[3] = buffer[iterator++];

  unsigned int ackNumber = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | (bytes[3]);
  //cout<<"Receiver says ack - "<<ackNumber<<endl;
  //cout<<acknowledgementNumber<<endl;
  bytes[0] = buffer[iterator++];
  bytes[1] = buffer[iterator++];
  bytes[2] = buffer[iterator++];
  bytes[3] = buffer[iterator++];

  unsigned int recvWin = (bytes[0] << 8) | (bytes[1]);
  //cout<<receiveWindow<<endl;
  string file_name = "";
  while(buffer[iterator] != '\0'){
    file_name+=string(1, buffer[iterator++]);
    acknowledgementNumber++;
  }
  return file_name;
}

void readFile(char* file_data, string file_name, int& file_size){
  ifstream file_id(file_name.c_str());

  if(!file_id){
    cout<<"Could not find "<<file_name<<endl;
    file_size = -1;
    return;
  }
  else{
    string temp = "";
    int iterator = 0;
    while(getline(file_id, temp)){
      temp+="\n";
      memcpy(file_data+iterator, temp.c_str(), temp.size());
      iterator+=temp.size();
    }
    file_size = iterator;
    file_id.close();
  }
}

int generateResponse(char* buffer, char* file_data, int file_size, int byte_index, unsigned int sequenceNumber, unsigned int acknowledgementNumber, unsigned int receiveWindow, int prev_bytes){
  unsigned char bytes[4];
  bytes[0] = ((sequenceNumber + prev_bytes)>> 24) & 0XFF;
  bytes[1] = ((sequenceNumber + prev_bytes) >> 16) & 0XFF;
  bytes[2] = ((sequenceNumber + prev_bytes) >> 8) & 0XFF;
  bytes[3] = (sequenceNumber + prev_bytes) & 0XFF;



  memcpy(buffer, bytes, 4);


  bytes[0] = (acknowledgementNumber >> 24) & 0XFF;
  bytes[1] = (acknowledgementNumber >> 16) & 0XFF;
  bytes[2] = (acknowledgementNumber >> 8) & 0XFF;
  bytes[3] = acknowledgementNumber & 0XFF;

  memcpy(buffer+4, bytes, 4);
  bytes[0] = (receiveWindow >> 8) & 0XFF;
  bytes[1] = receiveWindow & 0XFF;
  bytes[2] = 0;
  bytes[3] = 0;

  memcpy(buffer+8, bytes, 4);

  int cur_size = min(DATASIZE, file_size - byte_index);
  memcpy(buffer+12, file_data+byte_index+prev_bytes, cur_size);
  //cout<<"Seq number for sending -- "<<sequenceNumber<<endl;
  //byte_index+=cur_size;
  return cur_size;
}


void closeConnection(char* buffer, char* file_data, unsigned int sequenceNumber, unsigned int acknowledgementNumber, int server_fd, struct sockaddr_in& remaddr, socklen_t raddrlen){
  bzero(file_data, MAXFILESIZE);
  generateResponse(buffer, file_data, MAXFILESIZE, 0, sequenceNumber, acknowledgementNumber, RECVWINDOW, 0);
  bool is_connection_terminated = false;
  while(!is_connection_terminated){
    if(sendto(server_fd, buffer, BUFFSIZE, 0, (struct sockaddr*)&remaddr, raddrlen) < 0){
      perror("Error:");
      cout<<"Sending Failed"<<endl;
      close(server_fd);
      free(buffer);
      exit(EXIT_FAILURE);
    }

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(server_fd, &fds);

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 10000;

    if(select(server_fd+1, &fds, NULL, NULL, &tv) == 0){
      //cout<<"Failed to receive acknowledgement for connection termination packet."<<endl;
    }
    else{
      if(recvfrom(server_fd, buffer, BUFFSIZE, 0, (struct sockaddr*)&remaddr, &raddrlen) < 0){
        cout<<"Failed to read the socket buffer."<<endl;
      }
      else{
          bzero(buffer, BUFFSIZE);
          is_connection_terminated = true;
          generateResponse(buffer, file_data, MAXFILESIZE, 0, sequenceNumber, acknowledgementNumber, RECVWINDOW, 0);
          buffer[11] = buffer[11] | 2;
          cout<<"Sending final termination"<<endl;
          if(sendto(server_fd, buffer, BUFFSIZE, 0, (struct sockaddr*)&remaddr, raddrlen) < 0){
            perror("Error:");
            cout<<"Sending Failed"<<endl;
            close(server_fd);
            free(buffer);
            exit(EXIT_FAILURE);
          }
      }
    }
  }
}

int main(int argc, char const* argv[]){
    if(argc < 3){
      cout<<"Please pass the port number and advertised window size as parameter."<<endl;
      exit(EXIT_FAILURE);
    }
    if(argc > 3){
      cout<<"Program accepts only parameter which is the port number."<<endl;
      exit(EXIT_FAILURE);
    }
    int portNumber = atoi(argv[1]);
    int advertisedWindow = atoi(argv[2]);
    int threshold = advertisedWindow;
    int server_fd;
    if((server_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
      cout<<"Socket creating failed."<<endl;
      exit(EXIT_FAILURE);
    }

    int opt = 1;
    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
      cout<<"Error Assigning port."<<endl;
      exit(EXIT_FAILURE);
    }

    struct sockaddr_in address;
    memset((char*)&address, 0, sizeof(address));

    int addlen = sizeof(address);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(portNumber);

    if(bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0){
      cout<<"Binding Failed."<<endl;
      exit(EXIT_FAILURE);
    }

    int recvlen;
    char* buffer = (char*)calloc(BUFFSIZE, sizeof(char));
    char* file_data = (char*)calloc(MAXFILESIZE, sizeof(char));
    struct sockaddr_in remaddr;
    socklen_t raddrlen = sizeof(remaddr);

    while(1){
      cout<<"Receiving on port: "<<portNumber<<endl;
      unsigned int sequenceNumber = 0;
      unsigned int acknowledgementNumber = 0;
      unsigned int receiveWindow = RECVWINDOW;
      recvlen = recvfrom(server_fd, buffer, BUFFSIZE, 0, (struct sockaddr*)&remaddr, &raddrlen);


      string file_name = parseRequest(buffer, acknowledgementNumber);
      bzero(buffer, BUFFSIZE);
      bzero(file_data, MAXFILESIZE);
      int file_size = 0;
      readFile(file_data, file_name, file_size);

      fd_set fds;
      struct timeval tv;
      int byte_index = 0;

      //set intial timeout to 1000000 microsecs;
      tv.tv_sec = 0;
      tv.tv_usec = 900000;
      cwnd = 1;
      duplicate_count = 0;
      threshold = advertisedWindow;
      cout<<"File size: "<<file_size<<endl;
      while(byte_index < file_size){

        //Start the timer for measuring RTT -
        auto start_timer = std::chrono::high_resolution_clock::now();


            int prev_bytes = 0;
            int packet = 0;
            while(packet < min(cwnd, advertisedWindow) && (byte_index + prev_bytes) < file_size){
                prev_bytes += generateResponse(buffer, file_data, file_size, byte_index, sequenceNumber, acknowledgementNumber, receiveWindow, prev_bytes);
                if(sendto(server_fd, buffer, BUFFSIZE, 0, (struct sockaddr*)&remaddr, raddrlen) <= 0){
                  perror("Error:");
                  cout<<"Sending Failed"<<endl;
                  close(server_fd);
                  free(buffer);
                  exit(EXIT_FAILURE);
                }
                bzero(buffer, BUFFSIZE);
                packet++;
            }
            FD_ZERO(&fds);
            FD_SET(server_fd, &fds);

            if(select(server_fd+1, &fds, NULL, NULL, &tv) == 0){
              //cout<<"Failed to receive acknowledgement. Retransmitting the packet."<<endl;
              //perror("Error: ");
              if(!slow_start){
                cout<<endl;
                cout<<"Time out event has occurred. Changing the state to Slow Start"<<endl;
                cout<<endl;
                slow_start = true;
              }
              threshold = cwnd/2;
              cwnd = 1;
              duplicate_count = 0;
            }
            else{
                if(recvfrom(server_fd, buffer, BUFFSIZE, 0, (struct sockaddr*)&remaddr, &raddrlen) < 0){
                cout<<"Failed to read the socket buffer."<<endl;
                }
                else{
                  count_of_packets++;
                  if(slow_start){
                    slow_start_count++;
                  }
                  else if(congestion_avoidance){
                    congestion_avoidance_count++;
                  }
                  else if(fast_recovery){
                    fast_recovery_count++;
                  }

                  //Ack received, end timer
                  auto end_timer = std::chrono::high_resolution_clock::now() - start_timer;
                  long long elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end_timer).count();

                  //Calculate timeout value using Jacobson/Karels Algorithm -
                  if (estimated_RTT == 0)
                      estimated_RTT = elapsed;
                  //set timeout for next packet -
                  tv.tv_usec = calculate_timeout(elapsed);

                  parseAcknowledgement(buffer, sequenceNumber, acknowledgementNumber, byte_index, prev_bytes);
                }
                bzero(buffer, BUFFSIZE);
            }
      }


      closeConnection(buffer, file_data, sequenceNumber, acknowledgementNumber, server_fd, remaddr, raddrlen);
      cout<<"File "<<file_name<<" sent successfully to the Client"<<endl;
      cout<<"Connection summary - "<<endl;
      cout<<"--------------------------"<<endl;
      cout<<"Total Number of packets sent - "<<count_of_packets<<endl;
      cout<<"Number of packets transferred in slow start phase - "<<slow_start_count<<endl;
      cout<<"Percentage of packets transferred in slow start phase - "<<((float)slow_start_count/count_of_packets)<<endl;
      cout<<endl;
      cout<<"Number of packets transferred in Congestion Avoidance phase - "<<congestion_avoidance_count<<endl;
      cout<<"Percentage of packets transferred in Congestion Avoidance phase  - "<<((float)congestion_avoidance_count/count_of_packets)<<endl;
      cout<<endl;
      cout<<"Number of packets transferred in Fast Recovery phase - "<<fast_recovery_count<<endl;
      cout<<"Percentage of packets transferred in Fast Recovery phase - "<<((float)fast_recovery_count/count_of_packets)<<endl;
      cout<<"--------------------------"<<endl;
      cout<<endl;
      bzero(buffer, BUFFSIZE);
    }
    close(server_fd);
    free(buffer);
    free(file_data);

}
