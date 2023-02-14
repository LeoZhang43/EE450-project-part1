# Makefile tools for project, need C++11
.PHONY: all
all: servermain client

servermain : servermain.cpp 
	g++ -Wall -O2 -std=c++11 -lstdc++ -pthread servermain.cpp -o servermain
client : client.cpp 
	g++ -Wall -O2 -std=c++11 -lstdc++ -pthread client.cpp -o client 

clean:
	rm -f servermain client