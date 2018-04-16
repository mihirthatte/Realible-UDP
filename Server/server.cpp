#include "headers.h"

//using namespace std;
long long calculate_timeout(long long sample_RTT){
    sample_RTT -= estimated_RTT>>3;
    estimated_RTT += sample_RTT;
    if (sample_RTT < 0 )
        sample_RTT = - 1 * sample_RTT;
    sample_RTT -= deviation >> 3;
    deviation += sample_RTT;
    return ((estimated_RTT >> 3) + (deviation >> 1));
}


void parseAcknowledgement(char* buffer, unsigned int& sequenceNumber, unsigned int& acknowledgementNumber, unsigned int& byte_index, int prev_bytes){
  unsigned char bytes[4];
  int iterator = 0;
  bytes[0] = buffer[iterator++];
  bytes[1] = buffer[iterator++];
  bytes[2] = buffer[iterator++];
  bytes[3] = buffer[iterator++];
  unsigned int seqNum = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | (bytes[3]);

  bytes[0] = buffer[iterator++];
  bytes[1] = buffer[iterator++];
  bytes[2] = buffer[iterator++];
  bytes[3] = buffer[iterator++];

  unsigned int ackNum = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | (bytes[3]);

  bytes[0] = buffer[iterator++];
  bytes[1] = buffer[iterator++];
  bytes[2] = buffer[iterator++];
  bytes[3] = buffer[iterator++];

  unsigned int recvWin = (bytes[0] << 8) | (bytes[1]);

  if((bytes[3] & 1) == 0){
    return;
  }
  if(acknowledgementNumber != seqNum){
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

  bytes[0] = buffer[iterator++];
  bytes[1] = buffer[iterator++];
  bytes[2] = buffer[iterator++];
  bytes[3] = buffer[iterator++];

  unsigned int ackNumber = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | (bytes[3]);

  bytes[0] = buffer[iterator++];
  bytes[1] = buffer[iterator++];
  bytes[2] = buffer[iterator++];
  bytes[3] = buffer[iterator++];

  unsigned int recvWin = (bytes[0] << 8) | (bytes[1]);

  string file_name = "";
  while(buffer[iterator] != '\0'){
    file_name+=string(1, buffer[iterator++]);
    acknowledgementNumber++;
  }
  return file_name;
}

int generateResponse(char* buffer, char* file_data, int file_size, unsigned int byte_index, unsigned int sequenceNumber, unsigned int acknowledgementNumber, unsigned int receiveWindow, int prev_bytes){
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

  int cur_size = min(DATASIZE, file_size - (int)(byte_index - baseNumber));
  memcpy(buffer+12, file_data+((int)(byte_index - baseNumber)+prev_bytes), cur_size);

  for(int index = 12; index < BUFFSIZE; index++){
    if(buffer[index] == '\0') break;
  }

  return cur_size;
}


void closeConnection(char* buffer, char* file_data, unsigned int sequenceNumber, unsigned int acknowledgementNumber, int server_fd, struct sockaddr_in& remaddr, socklen_t raddrlen, bool file_not_found){
  bzero(file_data, MAXFILESIZE);
  generateResponse(buffer, file_data, MAXFILESIZE, 0, sequenceNumber, acknowledgementNumber, RECVWINDOW, 0);
  bool is_connection_terminated = false;
  if(file_not_found) buffer[11] = buffer[11] | 4;
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

    if(select(server_fd+1, &fds, NULL, NULL, &tv) != 0){
      if(recvfrom(server_fd, buffer, BUFFSIZE, 0, (struct sockaddr*)&remaddr, &raddrlen) < 0){
        cout<<"Failed to read the socket buffer."<<endl;
        perror("Error: ");
      }
      else{
          if((buffer[11]&2) > 0) is_connection_terminated = true;
          bzero(buffer, BUFFSIZE);
          generateResponse(buffer, file_data, MAXFILESIZE, 0, sequenceNumber, acknowledgementNumber, RECVWINDOW, 0);
          buffer[11] = buffer[11] | 2;
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
      /* Initializing connection summary parameters - */
      congestion_avoidance_count = 0;
      slow_start_count = 0;
      fast_recovery_count = 0;
      count_of_packets = 0;
      congestion_avoidance = false;
      fast_recovery = false;
      slow_start = true;

      cout<<"Receiving on port: "<<portNumber<<endl;
      unsigned int sequenceNumber = 4294962293;
      unsigned int acknowledgementNumber = 0;
      baseNumber = sequenceNumber;
      unsigned int receiveWindow = RECVWINDOW;
      recvlen = recvfrom(server_fd, buffer, BUFFSIZE, 0, (struct sockaddr*)&remaddr, &raddrlen);


      string file_name = parseRequest(buffer, acknowledgementNumber);
      bzero(buffer, BUFFSIZE);
      bzero(file_data, MAXFILESIZE);
      int file_size = 0;
      readFile(file_data, file_name, file_size);
      fd_set fds;
      struct timeval tv;
      unsigned int byte_index = sequenceNumber;
      
      //set intial timeout to 900000 microsecs;
      tv.tv_sec = 0;
      tv.tv_usec = 900000;
      cwnd = 1;
      duplicate_count = 0;
      threshold = advertisedWindow;
      while((int)(byte_index - baseNumber) < file_size){

        //Start the timer for measuring RTT -
        auto start_timer = std::chrono::high_resolution_clock::now();


            int prev_bytes = 0;
            int packet = 0;
            while(packet < min(cwnd, advertisedWindow) && ((int)(byte_index - baseNumber) + prev_bytes) < file_size){
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

      bool file_not_found = file_size == -1 ? true : false;
      closeConnection(buffer, file_data, sequenceNumber, acknowledgementNumber, server_fd, remaddr, raddrlen, file_not_found);
      if(!file_not_found){
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
      }

      bzero(buffer, BUFFSIZE);
    }
    close(server_fd);
    free(buffer);
    free(file_data);

}
