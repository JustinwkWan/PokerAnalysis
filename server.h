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
#include "poker.h"

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
  CHECK,
  UNKNOWN
};

struct Player {
  std::string username;
  int server_fd;
  int gameRoomID; 
  bool hasHand; 
  int chips;
  int currentBet; 
  bool isActive;
  Card holecard1; 
  Card holecard2; 
};

struct GameRoom {
  int gameID; 
  int smallBlind;
  int bigBlind;
  std::vector<Player> players;
  std::unordered_map<string, int> playerIndex; // username -> index in players vector
  Poker* poker = new Poker();
  int currentPlayerIndex; 
  int Pot; 
  int currentBet; 
  int buttonPosition; 
  std::string gameStages; // Preflop, Flop, Turn, River
  vector<Card> communityCards; 
};

class Server {
public:
  Server();
  ~Server();
  void run();  
  
private:
  int server_fd;
  std::vector<int> freeGameID;
  int nextGameID;
  std::map<int, GameRoom> gameRooms; 
  std::map<std::string, int> registeredPlayers;
  std::unordered_map<string, int> playerToGameID; // username -> game_id
  
  void handleClient(int client_fd);  // MODIFIED: Now takes client_fd as parameter
  void handleRegister(const json& request, int client_fd, std::string& client_username);
  void handleListGames(const json& request, int client_fd);
  void handleCreateGame(const json& request, int client_fd);
  void handleJoinGame(const json& request, int client_fd);
  void handleExitGame(const json& request, int client_fd);
  void handleUnregister(const json& request, int client_fd, std::string& client_username);
  bool handleAction(const json& request, int client_fd);
  
  json receiveMessage(int clientSocket);
  void sendMessage(int clientSocket, const json& data);
  MessageType getMessageType(const std::string& typeStr);
  ActionType getActionType(const std::string& typeStr);
};

#endif // SERVER_H