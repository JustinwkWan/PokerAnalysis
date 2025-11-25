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
class Server {
public:
  Server();
  ~Server();
  void handleClient();
  
  
private:
  int server_fd;
  int client_fd;
  std::map<std::string, int> registeredPlayers; // username -> socket_fd
  std::vector<json> gameRooms; // List of game rooms represented as JSON objects
  
  json receiveMessage(int clientSocket);
  void sendMessage(int clientSocket, const json& data);

  void handleRegister(const json& request);
  void handleListGames(const json& request);
  void handleCreateGame(const json& request);


  MessageType getMessageType(const std::string& typeStr);
};

#endif // SERVER_H