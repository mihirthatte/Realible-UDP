#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstdlib>

#define BUFFSIZE 1456

using namespace std;

int main(int argc, char const* argv[]){
  if(argc != 4){
    cout<<"Invalid number of parameters."<<endl;
    exit(EXIT_FAILURE);
  }
}
