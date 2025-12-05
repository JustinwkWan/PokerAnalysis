# Texas Hold'em Poker Server - Makefile

all:
	g++ -std=c++17 -Wall -Wextra -g -pthread main.cpp server.cpp poker.cpp -o poker_server -lssl -lcrypto

clean:
	rm -f poker_server

run: all
	./poker_server

.PHONY: all clean run