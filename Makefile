# Texas Hold'em Poker Server - Makefile

all:
	g++ server.cpp -o server -I/opt/homebrew/include -L/opt/homebrew/lib -lssl -lcrypto -std=c++17
clean:
	rm -f poker_server

run: all
	./poker_server

.PHONY: all clean run