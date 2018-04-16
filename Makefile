all:
	g++ -c Server/file_reader.cpp -o Server/file_reader.o -std=c++11
	g++ -c Server/server.cpp -o Server/server.o -std=c++11
	g++ Server/file_reader.o Server/server.o -o Server/server_program -std=c++11
	g++ Client/client.cpp -o Client/client -std=c++11
