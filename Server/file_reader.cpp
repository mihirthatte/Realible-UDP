#include <iostream>
#include <fstream>
#include <cstring>

using namespace std;
void readFile(char* file_data, std::string file_name, int& file_size){
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
      std::memcpy(file_data+iterator, temp.c_str(), temp.size());
      iterator+=temp.size();
    }
    file_size = iterator;
    file_id.close();
  }
}
