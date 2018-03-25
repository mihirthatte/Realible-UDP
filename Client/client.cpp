#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdlib>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <fstream>

#define BUFFSIZE 1472
#define RECVWINDOW 65535
#define TIMEOUT_MS 100000
#define TIMEOUT_S 5
#define TIMEOUT_US 10000
#define MAXFILESIZE 10000000

using namespace std;


void generateRequest(string file_name, char* buffer, unsigned int& sequenceNumber, unsigned int acknowledgementNumber, unsigned int receiveWindow){
  unsigned char bytes[4];
  bytes[0] = (sequenceNumber >> 24) & 0XFF;
  bytes[1] = (sequenceNumber >> 16) & 0XFF;
  bytes[2] = (sequenceNumber >> 8) & 0XFF;
  bytes[3] = sequenceNumber & 0XFF;



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
  memcpy(buffer+12, file_name.c_str(), file_name.size()+1);
  sequenceNumber+=(file_name.size());

}

bool checkEndPacket(char* buffer){
  for(int index = 12; index < BUFFSIZE; index++){
    if(buffer[index] != 0) return false;
  }
  return true;
}

void copyBufferData(char* file_data, char* buffer, unsigned int& sequenceNumber, unsigned int& acknowledgementNumber){
  unsigned char bytes[4];
  int it = 0;
  bytes[0] = buffer[it++];
  bytes[1] = buffer[it++];
  bytes[2] = buffer[it++];
  bytes[3] = buffer[it++];

  unsigned int seqNum = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | (bytes[3]);

  bytes[0] = buffer[it++];
  bytes[1] = buffer[it++];
  bytes[2] = buffer[it++];
  bytes[3] = buffer[it++];

  unsigned int ackNumber = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | (bytes[3]);

  bytes[0] = buffer[it++];
  bytes[1] = buffer[it++];
  bytes[2] = buffer[it++];
  bytes[3] = buffer[it++];

  unsigned int recvWin = (bytes[0] << 8) | (bytes[1]);

  if(seqNum < acknowledgementNumber) return;

  if(acknowledgementNumber != seqNum){
    cout<<"Server sequence number did not match with the client acknowledgement number"<<endl;
    return;
  }

  //Need to put a check for client sequence number check for server acknowledgement

  for(int index = 12; index < BUFFSIZE; index++){
    if(buffer[index] == '\0') break;
    file_data[acknowledgementNumber++] = buffer[index];
  }
  cout<<acknowledgementNumber<<" "<<seqNum<<endl;
}

void generateAcknowledgement(char* buffer, unsigned int& sequenceNumber, unsigned int& acknowledgementNumber, unsigned int receiveWindow){
  unsigned char bytes[4];
  bytes[0] = (sequenceNumber >> 24) & 0XFF;
  bytes[1] = (sequenceNumber >> 16) & 0XFF;
  bytes[2] = (sequenceNumber >> 8) & 0XFF;
  bytes[3] = sequenceNumber & 0XFF;



  memcpy(buffer, bytes, 4);


  bytes[0] = (acknowledgementNumber >> 24) & 0XFF;
  bytes[1] = (acknowledgementNumber >> 16) & 0XFF;
  bytes[2] = (acknowledgementNumber >> 8) & 0XFF;
  bytes[3] = acknowledgementNumber & 0XFF;

  memcpy(buffer+4, bytes, 4);
  bytes[0] = (receiveWindow >> 8) & 0XFF;
  bytes[1] = receiveWindow & 0XFF;
  bytes[2] = 0;
  bytes[3] = 1;

  memcpy(buffer+8, bytes, 4);
}

void closeConnection(char* buffer, unsigned int sequenceNumber, unsigned int acknowledgementNumber, int sock_id, struct sockaddr_in& server_address, socklen_t addrlen){
  bool is_connection_terminated = false;
  while(!is_connection_terminated){
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(sock_id, &fds);
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 10000;
    if(select(sock_id+1, &fds, NULL, NULL, &tv) == 0){
      cout<<"Timeout on receiving packet."<<endl;
      is_connection_terminated = true;;
    }
    else{
      if(recvfrom(sock_id, buffer, BUFFSIZE, 0, (struct sockaddr*)&server_address, &addrlen) < 0){
        cout<<"Error while receiving packet from client socket."<<endl;
        perror("Error: ");
        is_connection_terminated = true;
      }
      else{
        if((buffer[11] & 2) > 0){
          is_connection_terminated = true;
          cout<<"Terminated"<<endl;
        }
        else{
          unsigned char temp = (unsigned char)buffer[11];
          cout<<(int)temp<<endl;
          cout<<"Resending the termination."<<endl;
          bzero(buffer, BUFFSIZE);
          generateAcknowledgement(buffer, sequenceNumber, acknowledgementNumber, RECVWINDOW);

          if(sendto(sock_id, buffer, BUFFSIZE, 0, (struct sockaddr*)&server_address, addrlen) < 0){
        		perror("Error");
        		cout<<"Sending Failed"<<endl;
        	}
        }

      }
    }
  }
}

int main(int argc, char const* argv[]){
  if(argc != 6){
    cout<<"Invalid number of parameters."<<endl;
    exit(EXIT_FAILURE);
  }

  srand (time(NULL)); //Initializing random seed to null
  string server_host = string(argv[1]);
  int server_port = atoi(argv[2]);
  string file_name = string(argv[3]);
  int advertisedWindow = atoi(argv[4]);
  int drop_probability = atoi(argv[5]);

  int sock_id;

  struct sockaddr_in server_address;
  socklen_t addrlen = sizeof(server_address);
  if((sock_id = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
  	cout<<"Error in creating the socket."<<endl;
  	exit(EXIT_FAILURE);
  }


  /*
  struct timeval tv;
  tv.tv_sec = TIMEOUT_S;
  tv.tv_usec = 0;
  if (setsockopt(sock_id, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
      perror("Error");
  }
	*/

  memset((char*)&server_address, 0, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(server_port);

  if(inet_pton(AF_INET, server_host.c_str(), &server_address.sin_addr) < 0){
  	cout<<"Invalid address. This address is not supported."<<endl;
	   exit(EXIT_FAILURE);
  }

  unsigned int sequenceNumber = 0;
  unsigned int acknowledgementNumber = 0;
  unsigned int receiveWindow = RECVWINDOW;
  char* buffer = (char*)calloc(BUFFSIZE, sizeof(char));
  generateRequest(file_name, buffer, sequenceNumber, acknowledgementNumber, receiveWindow);


  if(sendto(sock_id, buffer, BUFFSIZE, 0, (struct sockaddr*) &server_address, addrlen) < 0){
    perror("Error");
    cout<<"Sending Failed"<<endl;
    close(sock_id);
    exit(EXIT_FAILURE);
  }

  char* file_data = (char*)calloc(MAXFILESIZE, sizeof(char));
  bzero(buffer, BUFFSIZE);
  bool is_end_of_file = false;
  //int iterator = 0;
  /*
  while(recvfrom(sock_id, buffer, BUFFSIZE, 0, (struct sockaddr*)&server_address, &addrlen) > 0){
    is_end_of_file = checkEndPacket(buffer);
    if(is_end_of_file) break;
    copyBufferData(file_data, buffer, iterator, sequenceNumber, acknowledgementNumber);
    bzero(buffer, BUFFSIZE);
    generateAcknowledgement(buffer, sequenceNumber, acknowledgementNumber, receiveWindow);

    if(sendto(sock_id, buffer, BUFFSIZE, 0, (struct sockaddr*) &server_address, addrlen) < 0){
      perror("Error");
      cout<<"Sending Failed"<<endl;
      close(sock_id);
      exit(EXIT_FAILURE);
    }
    bzero(buffer, BUFFSIZE);
  }
  */
  int windowSize = 10;

  fd_set fds;
  struct timeval tv;

  int packet_count = 0;
  while(!is_end_of_file){
  	int window_packet;
  	for(window_packet = 0; (window_packet < windowSize) && (!is_end_of_file); window_packet++){
  		bzero(buffer, BUFFSIZE);

  		FD_ZERO(&fds);
  		FD_SET(sock_id, &fds);

  		tv.tv_sec = 0;
  		tv.tv_usec = TIMEOUT_US;

  		if(select(sock_id+1, &fds, NULL, NULL, &tv) == 0){
  			cout<<"Timeout on receiving packet. Sending the last acknowledgement."<<endl;
  			break;
  		}
  		else{
  			if(recvfrom(sock_id, buffer, BUFFSIZE, 0, (struct sockaddr*)&server_address, &addrlen) < 0){
  				cout<<"Error while receiving packet from client socket."<<endl;
  				perror("Error: ");
  			}
  			else{
          /* Drop packet with drop_probabilty -
          */
          int num = rand()%100;
          if (num < drop_probability){ //packet was dropped
            cout<<num<<" Dropping the packet with drop probability of "<<drop_probability<<endl;
            break;
          }
          else{ //Packet was not dropped
            packet_count++;
            /*
            if (packet_count == 10){
              //cout<<"Sleeping for 20000 micro seconds"<<endl;
              usleep(20000);

              packet_count = 0;
            }
            */
    				is_end_of_file = checkEndPacket(buffer);
    				if(is_end_of_file){
              cout<<"End Packet received"<<endl;
              break;
            }
    				copyBufferData(file_data, buffer, sequenceNumber, acknowledgementNumber);
        }
  			}

  		}

  	}

  	bzero(buffer, BUFFSIZE);
  	generateAcknowledgement(buffer, sequenceNumber, acknowledgementNumber, receiveWindow);
  	if(sendto(sock_id, buffer, BUFFSIZE, 0, (struct sockaddr*)&server_address, addrlen) < 0){
  		perror("Error");
  		cout<<"Sending Failed"<<endl;
  	}

  	windowSize = min(window_packet + 1, advertisedWindow);
  }

  closeConnection(buffer, sequenceNumber, acknowledgementNumber, sock_id, server_address, addrlen );
  ofstream output_file(file_name);
  output_file<<file_data;
  output_file.close();

  close(sock_id);
  free(buffer);
}
