#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "json.hpp"
#include "poker.h"
#include <memory>

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
  std::vector<Card> holeCards;  // Changed: Use vector instead of two separate cards
};

struct GameRoom {
  int gameID; 
  int smallBlind;
  int bigBlind;
  std::vector<Player> players;
  std::unordered_map<string, int> playerIndex; // username -> index in players vector
  std::unique_ptr<Poker> poker;  // Use smart pointer for automatic memory management
  int currentPlayerIndex; 
  int Pot; 
  int currentBet; 
  int buttonPosition; 
  std::string gameStages; // WAITING, PREFLOP, FLOP, TURN, RIVER, SHOWDOWN
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
  
  // Client handling
  void handleClient(int client_fd);
  
  // Message handlers
  void handleRegister(const json& request, int client_fd, std::string& client_username);
  void handleListGames(const json& request, int client_fd);
  void handleCreateGame(const json& request, int client_fd);
  void handleJoinGame(const json& request, int client_fd);
  void handleExitGame(const json& request, int client_fd);
  void handleUnregister(const json& request, int client_fd, std::string& client_username);
  void handleStartGame(const json& request, int client_fd);  // Added: Start game handler
  bool handleAction(const json& request, int client_fd);
  
  // Game flow methods
  void advanceGameStage(GameRoom& room);  // Added: Advance through game stages
  void handleShowdown(GameRoom& room);     // Added: Handle showdown logic
  void broadcastGameState(GameRoom& room); // Added: Broadcast state to all players
  
  // Communication
  json receiveMessage(int clientSocket);
  void sendMessage(int clientSocket, const json& data);
  
  // Utility
  MessageType getMessageType(const std::string& typeStr);
  ActionType getActionType(const std::string& typeStr);
};

#endif // SERVER_H