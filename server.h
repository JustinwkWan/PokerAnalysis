#ifndef SERVER_H
#define SERVER_H
#include <string>
#include <map>
#include <unordered_map>
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

enum class ActionType {
  BET,
  FOLD,
  ALL_IN,
  CALL,
  RAISE,
  UNKNOWN
};

struct Player {
  std::string username;
  int server_fd;
  int gameRoomID; 
};

struct GameRoom {
  int gameID; 
  int smallBlind;
  int bigBlind;
  std::vector<Players> players;
  std::unordered_map<string, int> playerIndex; // NEW: username -> index in players vector
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
  
  std::unordered_map<string, int> playerToGameID; // username -> game_id
  
  void handleRegister(const json& request);
  void handleListGames(const json& request);
  void handleCreateGame(const json& request);
  void handleJoinGame(const json& request);
  void handleExitGame(const json& request);
  void handleUnregister(const json& request);
  json receiveMessage(int clientSocket);
  void sendMessage(int clientSocket, const json& data);
  void handleAction(const json& request);
  MessageType getMessageType(const std::string& typeStr);
  ActionType getActionType(const std::string& typeStr);
};
#endif // SERVER_H