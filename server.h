#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <map>
#include "json.hpp"
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include <arpa/inet.h>

using namespace std;

using json = nlohmann::json;
const int MAX_PLAYERS = 9;

enum class MessageType
{
  LOGIN,
  REGISTER,
  CREATE_GAME,
  JOIN_GAME,
  EXIT_GAME,
  EXIT_ROOM,
  UNREGISTER,
  START_GAME,
  PLAY_TURN,
  LIST_GAMES,
  UNKNOWN 
};

struct Players {
  std::string username;
  int server_fd;
};
struct GameRoom {
  int gameID; 
  int smallBlind;
  int bigBlind;
  std::vector<Players> players;
};



class Server {
public:
  Server();
  ~Server();
  void handleClient();
  
private:
  int server_fd;
  int client_fd;
  std::vector<int> freeGameID;
  int nextGameID;
  std::map<int, GameRoom> gameRooms; 
  std::map<std::string, int> registeredPlayers; 

  void handleRegister(const json& request);
  void handleListGames(const json& request);
  void handleCreateGame(const json& request);
  void handleJoinGame(const json& request);

  json receiveMessage(int clientSocket);
  void sendMessage(int clientSocket, const json& data);
  MessageType getMessageType(const std::string& typeStr);
};

#endif // SERVER_H