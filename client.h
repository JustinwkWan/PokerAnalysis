#ifndef CLIENT_H
#define CLIENT_H

#include <string>
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

enum class MESSAGETYPE
{
  LOGIN,
  REGISTER,
  CREATE_GAME,
  JOIN_GAME,
  EXIT_GAME,
  UNREGISTER,
  START_GAME,
  PLAY_TURN
};

struct PlayerInfo
{
  char name[50];
  char password[50];
  int gameID; // -1 if not in a game
  int socket_fd;
};

struct GameRoom
{
  int game_id;
  std::vector<int> player_fds;
  int max_players;
};

class client
{
public:
  client();
  ~client();
  bool Register(char *name);
  void ListGames();
  void createGame(const int smallBlind, 
  const int bigBlind);
  void joinGame(char* username, int gameID, int startingStack);
  void exitGame(char *username);
  void unRegister(char *username);
  void StartGame(int gameID);
  void PlayTurn(const char* username, int gameID, const string& action, int turnData);
private:
  int server_fd;
  std::string username;
  bool registered;
  
  void sendMessage(const json data);
  json recieveMessage();
};

#endif // CLIENT_H