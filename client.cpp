#include "client.h"

client::client()
{
  // creating socket
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    throw std::runtime_error("Failed to create socket");
  }
  
  // specifying address
  sockaddr_in serverAddress;
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(8080);
  serverAddress.sin_addr.s_addr = INADDR_ANY;
  
  // sending connection request
  if (connect(server_fd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
    throw std::runtime_error("Failed to connect to server");
  }
  
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
    throw std::runtime_error("Failed to send message length");
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
    cout << "Successfully registered " << name << endl;
    registered = true;
    username = name;
    return true;
  }
  else
  {
    cout << "Registration failed: " << response["error"] << endl;
    return false; 
  }
}

void client::ListGames()
{
  json request = {
      {"type", "LIST_GAMES"},
  };

  sendMessage(request);
  json response = recieveMessage();

  if (response["status"] != "SUCCESS") {
    cout << "Failed to list games: " << response["error"] << endl;
    return;
  }
  cout << "Available Games:" << endl;
  for (const auto& game : response["games"]) {
    cout << "Game ID: " << game["game_id"] 
    << ", Players: " << game["current_players"] 
    << ", Small/Big blind: " << game["small_blind"] 
    << "/" << game["big_blind"] << endl; 
  }
}

void client::createGame(
  const int smallBlind, 
  const int bigBlind,
  const int GameID)
{
  // TODO: Create a GameID generator to avoid collisions
  json request = {
      {"type", "CREATE_GAME"},
      {"small_blind", smallBlind},
      {"big_blind", bigBlind},
      {"game_id", GameID}
  };

  sendMessage(request);
  json response = recieveMessage();

  if (response["status"] == "SUCCESS") {
    cout << "Game created successfully with Game ID: " << GameID << endl;
  } else {
    cout << "Failed to create game: " << response["error"] << endl;
  }
}

void client::joinGame(int gameID)
{
  json request = {
      {"type", "JOIN_GAME"},
      {"game_id", gameID}
  };

  if(!registered) {
    cout << "You must register before joining a game." << endl;
    return;
  }
  sendMessage(request);
  json response = recieveMessage();

  if (response["status"] == "SUCCESS") {
    cout << "Joined game ID: " << gameID << " successfully." << endl;
  } else {
    cout << "Failed to join game: " << response["error"] << endl;
  }
}

void client::exitGame()
{
  json request = {
      {"type", "EXIT_GAME"},
  };

  sendMessage(request);
  json response = recieveMessage();

  if (response["status"] == "SUCCESS") {
    cout << "Exited game successfully." << endl;
  } else {
    cout << "Failed to exit game: " << response["error"] << endl;
  }
}

void client::exitRoom() {
  json request = {
      {"type", "EXIT_ROOM"},
  };
  sendMessage(request);
  json response = recieveMessage();
  if (response["status"] == "SUCCESS") {
    cout << "Exited room successfully." << endl;
  } else {
    cout << "Failed to exit room: " << response["error"] << endl;
  }
}

void client::StartGame(int gameID)
{
  // TODO: Implement
}

void client::PlayTurn(int gameID, int turnData)
{
  // TODO: Implement
}