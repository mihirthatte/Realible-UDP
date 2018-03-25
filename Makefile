all:
	g++ Server/server.cpp -o Server/server -std=c++11
	g++ Client/client.cpp -o Client/client -std=c++11
