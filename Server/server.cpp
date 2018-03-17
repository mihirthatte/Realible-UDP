#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <unistd.h>

#define DATASIZE 1456
#define BUFFSIZE 1568
#define RECVWINDOW 65535
#define MAXFILESIZE 10000000

using namespace std;

string parseRequest(char* buffer){
  unsigned char bytes[4];
  int iterator = 0;
  bytes[0] = buffer[iterator++];
  bytes[1] = buffer[iterator++];
  bytes[2] = buffer[iterator++];
  bytes[3] = buffer[iterator++];
  unsigned int sequenceNumber = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | (bytes[3]);
  cout<<sequenceNumber<<endl;
  bytes[0] = buffer[iterator++];
  bytes[1] = buffer[iterator++];
  bytes[2] = buffer[iterator++];
  bytes[3] = buffer[iterator++];

  unsigned int acknowledgementNumber = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | (bytes[3]);
  cout<<acknowledgementNumber<<endl;
  bytes[0] = buffer[iterator++];
  bytes[1] = buffer[iterator++];
  bytes[2] = buffer[iterator++];
  bytes[3] = buffer[iterator++];

  unsigned int receiveWindow = (bytes[0] << 8) | (bytes[1]);
  cout<<receiveWindow<<endl;
  string file_name = "";
  while(buffer[iterator] != '\0'){
    file_name+=string(1, buffer[iterator++]);
  }
  return file_name;
}

void readFile(char* file_data, string file_name, int& file_size){
  ifstream file_id(file_name.c_str());

  if(!file_id){
    cout<<"Could not find "<<file_name<<"."<<endl;
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

void generateResponse(char* buffer, char* file_data, int file_size, int& byte_index, unsigned int sequenceNumber, unsigned int acknowledgementNumber, unsigned int receiveWindow){
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

  int cur_size = min(DATASIZE, file_size - byte_index);
  memcpy(buffer+12, file_data+byte_index, cur_size);
  byte_index+=cur_size;
}

int main(int argc, char const* argv[]){
    if(argc < 2){
      cout<<"Please pass the port number as parameter."<<endl;
      exit(EXIT_FAILURE);
    }
    if(argc > 2){
      cout<<"Program accepts only parameter which is the port number."<<endl;
      exit(EXIT_FAILURE);
    }
    int portNumber = atoi(argv[1]);
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


      string file_name = parseRequest(buffer);
      bzero(buffer, BUFFSIZE);
      int file_size = 0;
      readFile(file_data, file_name, file_size);

      int byte_index = 0;
      while(byte_index < file_size){
        generateResponse(buffer, file_data, file_size, byte_index, sequenceNumber, acknowledgementNumber, receiveWindow);

        if(sendto(server_fd, buffer, BUFFSIZE, 0, (struct sockaddr*)&remaddr, raddrlen) < 0){
          perror("Error:");
          cout<<"Sending Failed"<<endl;
          close(server_fd);
          free(buffer);
          exit(EXIT_FAILURE);
        }
        bzero(buffer, BUFFSIZE);
        sequenceNumber+=byte_index;
      }

      memset(file_data, 0, MAXFILESIZE);
      int it = 0;
      generateResponse(buffer, file_data, MAXFILESIZE, it, sequenceNumber, acknowledgementNumber, receiveWindow);
      if(sendto(server_fd, buffer, BUFFSIZE, 0, (struct sockaddr*)&remaddr, raddrlen) < 0){
        perror("Error:");
        cout<<"Sending Failed"<<endl;
        close(server_fd);
        free(buffer);
        exit(EXIT_FAILURE);
      }
      bzero(buffer, BUFFSIZE);
      sequenceNumber+=1;
    }
    close(server_fd);
    free(buffer);

}
