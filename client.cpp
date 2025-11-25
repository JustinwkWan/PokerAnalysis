#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include "json.hpp"
using json = nlohmann::json;
using namespace std;
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
  void createGame();
  void joinGame(int gameID);
  void exitGame(int gameID);
  void unRegister(char *name, char *password);
  void StartGame(int gameID);
  void PlayTurn(int gameID, int turnData);

private:
  int server_fd;
  std::string username;
  bool registered;

  void sendMessage(const json data);
  json recieveMessage();
};

client::client()
{
  // creating socket
  server_fd = socket(AF_INET, SOCK_STREAM, 0);

  // specifying address
  sockaddr_in serverAddress;
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(8080);
  serverAddress.sin_addr.s_addr = INADDR_ANY;

  // sending connection request
  connect(server_fd, (struct sockaddr *)&serverAddress,
          sizeof(serverAddress));

  registered = false;
}

client::~client()
{
  // closing socket
  close(server_fd);
}

void client::sendMessage(const json data)
{
  // Serialize JSON to string
  string jsonString = data.dump();
  uint32_t length = htonl(jsonString.size());

  // Send the length of the JSON string first
  ssize_t bytesSent = send(server_fd, &length, sizeof(length), 0);
  if (bytesSent != sizeof(length))
  {
    cerr << "Error sending message length" << endl;
    return;
  }

  // Now send the actual JSON string
  bytesSent = send(server_fd, jsonString.c_str(), jsonString.length(), 0);
  if (bytesSent != (ssize_t)jsonString.length())
  {
    throw std::runtime_error("Failed to send message");
  }
}

json client::recieveMessage()
{
  // First, read the length of the incoming JSON string
  uint32_t length; 
  ssize_t bytesRead = recv(server_fd, &length, sizeof(length), MSG_WAITALL);
  if(bytesRead != sizeof(length))
  {
    throw std::runtime_error("Failed to read message length");
  }

  // Now read the actual JSON string based on the length
  uint32_t msg_length = ntohl(length);

  // create the buffer for the message
  vector<char> buffer(msg_length);

  bytesRead = recv(server_fd, buffer.data(), msg_length, MSG_WAITALL);
  if (bytesRead != (ssize_t)msg_length)
  {
    throw std::runtime_error("Failed to read complete message");
  }

  // Parse the JSON string
  string jsonString(buffer.begin(), buffer.end());
  return json::parse(jsonString);
}
bool client::Register(char *name)
{
  json request = {
      {"type", "REGISTER"},
      {"name", name},
  };

  sendMessage(request);

  json response = recieveMessage();

  if (response["status"] == "SUCCESS")
  {
    cout << "Registration successful!" << endl;
    registered = true;
    username = name;
  }
  else
  {
    cout << "Registration failed: " << response["error"] << endl;
  }
}

int main()
{
  client myClient;
  myClient.Register("PlayerOne");
  return 0;
}