#include "server.h"
#include <thread>
#include <mutex>
#include <algorithm>

// Global mutex for thread safety
std::mutex server_mutex;

Server::Server()
{
  // Create socket
  nextGameID = 1;
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == 0)
  {
    cerr << "Socket creation error" << endl;
    exit(EXIT_FAILURE);
  }

  // Set socket options
  int opt = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                 &opt, sizeof(opt)))
  {
    cerr << "setsockopt error" << endl;
    exit(EXIT_FAILURE);
  }

  // Setup address
  sockaddr_in serverAddress;
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(8080);
  serverAddress.sin_addr.s_addr = INADDR_ANY;

  // Bind socket
  if (bind(server_fd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
  {
    cerr << "Bind failed" << endl;
    exit(EXIT_FAILURE);
  }

  // Listen
  if (listen(server_fd, 10) < 0)
  {
    cerr << "Listen failed" << endl;
    exit(EXIT_FAILURE);
  }

  cout << "Server started on port 8080, waiting for connections..." << endl;
}

Server::~Server()
{
  close(server_fd);
}

MessageType Server::getMessageType(const std::string &typeStr)
{
  if (typeStr == "LOGIN")              return MessageType::LOGIN;
  if (typeStr == "REGISTER")           return MessageType::REGISTER;
  if (typeStr == "CREATE_GAME")        return MessageType::CREATE_GAME;
  if (typeStr == "JOIN_GAME")          return MessageType::JOIN_GAME;
  if (typeStr == "EXIT_GAME")          return MessageType::EXIT_GAME;
  if (typeStr == "UNREGISTER")         return MessageType::UNREGISTER;
  if (typeStr == "START_GAME")         return MessageType::START_GAME;
  if (typeStr == "PLAY_TURN")          return MessageType::PLAY_TURN;
  if (typeStr == "LIST_GAMES")         return MessageType::LIST_GAMES;
  return MessageType::UNKNOWN;
}

ActionType Server::getActionType(const std::string &typeStr) {
  if(typeStr == "ALL_IN")       return ActionType::ALL_IN;
  if(typeStr == "FOLD")         return ActionType::FOLD;
  if(typeStr == "CALL")         return ActionType::CALL;
  if(typeStr == "CHECK")        return ActionType::CHECK;
  if(typeStr == "RAISE")        return ActionType::RAISE;
  if(typeStr == "BET")          return ActionType::BET;
  return ActionType::UNKNOWN;
}

json Server::receiveMessage(int clientSocket)
{
  uint32_t msg_length_net;
  ssize_t received = recv(clientSocket, &msg_length_net, sizeof(msg_length_net), MSG_WAITALL);
  if (received <= 0)
  {
    throw std::runtime_error("Client disconnected");
  }

  uint32_t msg_length = ntohl(msg_length_net);
  
  // Sanity check on message length
  if (msg_length > 1000000) { // 1MB max
    throw std::runtime_error("Message too large");
  }
  
  std::vector<char> buffer(msg_length);

  received = recv(clientSocket, buffer.data(), msg_length, MSG_WAITALL);
  if (received <= 0)
  {
    throw std::runtime_error("Client disconnected");
  }

  std::string json_str(buffer.begin(), buffer.end());
  return json::parse(json_str);
}

void Server::sendMessage(int clientSocket, const json &message)
{
  std::string jsonString = message.dump();
  uint32_t length = htonl(jsonString.size());

  ssize_t bytesSent = send(clientSocket, &length, sizeof(length), 0);
  if (bytesSent != sizeof(length))
  {
    throw std::runtime_error("Failed to send message length");
  }

  bytesSent = send(clientSocket, jsonString.c_str(), jsonString.length(), 0);
  if (bytesSent != (ssize_t)jsonString.length())
  {
    throw std::runtime_error("Failed to send message");
  }
}

void Server::run()
{
  while (true)
  {
    // Accept new client connection
    sockaddr_in clientAddress;
    socklen_t clientLen = sizeof(clientAddress);
    int client_fd = accept(server_fd, (struct sockaddr *)&clientAddress, &clientLen);
    
    if (client_fd < 0)
    {
      cerr << "Accept failed" << endl;
      continue;
    }

    cout << "New client connected: fd=" << client_fd << endl;

    // Create a new thread to handle this client
    std::thread(&Server::handleClient, this, client_fd).detach();
  }
}

void Server::handleClient(int client_fd)
{
  std::string client_username = "";
  
  try
  {
    while (true)
    {
      json request = receiveMessage(client_fd);

      cout << "\n=== Received Message from fd=" << client_fd << " ===" << endl;
      cout << request.dump(2) << endl;
      
      MessageType msgType = getMessageType(request["type"]);

      switch (msgType)
      {
      case MessageType::REGISTER:
        handleRegister(request, client_fd, client_username);
        break;
      case MessageType::LIST_GAMES:
        handleListGames(request, client_fd);
        break;
      case MessageType::CREATE_GAME:
        handleCreateGame(request, client_fd);
        break;
      case MessageType::JOIN_GAME:
        handleJoinGame(request, client_fd);
        break;
      case MessageType::EXIT_GAME:
        handleExitGame(request, client_fd);
        break;
      case MessageType::UNREGISTER:
        handleUnregister(request, client_fd, client_username);
        break;
      case MessageType::PLAY_TURN:
        handleAction(request, client_fd);
        break;
      default:
        cout << "Unknown message type received." << endl;
        break;
      }
    }
  }
  catch (const std::exception &e)
  {
    cerr << "Client handling error (fd=" << client_fd << "): " << e.what() << endl;
    
    // Cleanup on disconnect
    std::lock_guard<std::mutex> lock(server_mutex);
    
    if (!client_username.empty())
    {
      // Remove from game if in one
      auto playerGameIt = playerToGameID.find(client_username);
      if (playerGameIt != playerToGameID.end())
      {
        int game_id = playerGameIt->second;
        auto roomIt = gameRooms.find(game_id);
        
        if (roomIt != gameRooms.end())
        {
          GameRoom& room = roomIt->second;
          auto idIt = room.playerIndex.find(client_username);
          
          if (idIt != room.playerIndex.end())
          {
            int playerIdx = idIt->second;
            
            // Fix: Save the username before modifying the vector
            if (playerIdx < room.players.size() - 1) {
              std::string movedPlayerName = room.players.back().username;
              room.players[playerIdx] = room.players.back();
              room.playerIndex[movedPlayerName] = playerIdx;
            }
            room.players.pop_back();
            room.playerIndex.erase(client_username);
          }
          
          if (room.players.empty()) {
            freeGameID.push_back(game_id);
            gameRooms.erase(game_id);
          }
        }
        
        playerToGameID.erase(client_username);
      }
      
      // Unregister player
      registeredPlayers.erase(client_username);
      cout << "Cleaned up disconnected client: " << client_username << endl;
    }
    
    close(client_fd);
  }
}

void Server::handleRegister(const json &request, int client_fd, std::string &client_username)
{
  std::lock_guard<std::mutex> lock(server_mutex);
  
  string username = request["name"];

  // Check if username is empty
  if(username.empty()) {
    json response = {
      {"type", "REGISTER_RESPONSE"},
      {"status", "ERROR"},  // Fixed: was missing closing quote
      {"error", "empty username"}
    };
    sendMessage(client_fd, response);
    return; 
  }
  
  // Check if username already taken
  if (registeredPlayers.find(username) != registeredPlayers.end())
  {
    json response = {
        {"type", "REGISTER_RESPONSE"},
        {"status", "ERROR"},
        {"error", "Username already taken"}};
    sendMessage(client_fd, response);
    return;
  }

  // Register the player
  registeredPlayers[username] = client_fd;
  client_username = username;

  json response = {
      {"type", "REGISTER_RESPONSE"},
      {"status", "SUCCESS"},
      {"message", "Welcome " + username}};

  sendMessage(client_fd, response);
  cout << "Registration successful for: " << username << " (fd=" << client_fd << ")" << endl;
  cout << "Total registered players: " << registeredPlayers.size() << endl;
}

void Server::handleListGames(const json &request, int client_fd)
{
  std::lock_guard<std::mutex> lock(server_mutex);
  
  json response = {
      {"type", "LIST_GAMES_RESPONSE"},
      {"status", "SUCCESS"},
      {"games", json::array()}
  };

  if(gameRooms.empty())
  {
    response["message"] = "No active game rooms available.";
    sendMessage(client_fd, response);
    return;
  }

  // Add each game to the response
  for (const auto &game : gameRooms)
  {
    response["games"].push_back({
      {"game_id", game.second.gameID},
      {"small_blind", game.second.smallBlind},
      {"big_blind", game.second.bigBlind},
      {"player_count", game.second.players.size()}
    });
  }
  
  sendMessage(client_fd, response);
}

void Server::handleCreateGame(const json &request, int client_fd)
{
  std::lock_guard<std::mutex> lock(server_mutex);
  
  // Check required parameters first
  if (!request.contains("small_blind") || !request.contains("big_blind"))
  {
    json response = {
      {"type", "CREATE_GAME_RESPONSE"},
      {"status", "ERROR"},
      {"error", "Missing game parameters"}
    };
    sendMessage(client_fd, response);
    cout << "Failed to create game: Missing parameters" << endl;
    return;
  }
  
  int smallBlind = request["small_blind"];
  int bigBlind = request["big_blind"];
  
  // Validate blinds
  if (smallBlind <= 0 || bigBlind <= 0)
  {
    json response = {
      {"type", "CREATE_GAME_RESPONSE"},
      {"status", "ERROR"},
      {"error", "Blinds must be positive values"}
    };
    sendMessage(client_fd, response);
    return;
  }
  
  // Small blind must be less than big blind
  if (smallBlind >= bigBlind)
  {
    json response = {
      {"type", "CREATE_GAME_RESPONSE"},
      {"status", "ERROR"},
      {"error", "Small blind must be less than big blind"}
    };
    sendMessage(client_fd, response);
    return;
  }

  // Generate game ID automatically
  int game_id;
  if (!freeGameID.empty()) {
    game_id = freeGameID.back();
    freeGameID.pop_back();
  } else {
    game_id = nextGameID++;
  }

  // Create GameRoom struct
  GameRoom newRoom;
  newRoom.gameID = game_id;
  newRoom.smallBlind = smallBlind;
  newRoom.bigBlind = bigBlind;
  newRoom.currentPlayerIndex = 0;
  newRoom.Pot = 0;
  newRoom.currentBet = 0;
  newRoom.buttonPosition = 0;
  newRoom.gameStages = "WAITING";
  
  gameRooms[game_id] = newRoom;
  
  json response = {
    {"type", "CREATE_GAME_RESPONSE"},
    {"status", "SUCCESS"},
    {"game_id", game_id},
    {"message", "Game created successfully"}
  };
  
  sendMessage(client_fd, response);
  cout << "Game created with ID: " << game_id << endl;
}

void Server::handleJoinGame(const json &request, int client_fd) 
{
  std::lock_guard<std::mutex> lock(server_mutex);
  
  int game_id = request["game_id"];
  string username = request["username"];
  
  json response = {{"type", "JOIN_GAME_RESPONSE"}};

  // Check if player is registered
  if (registeredPlayers.find(username) == registeredPlayers.end()) {
    response["status"] = "ERROR";
    response["error"] = "Invalid Player ID";
    sendMessage(client_fd, response);
    return;
  }
  
  // Check if player already in a game
  if (playerToGameID.find(username) != playerToGameID.end()) {
    response["status"] = "ERROR";
    response["error"] = "Player already in a game";
    sendMessage(client_fd, response);
    return;
  }
  
  // Find game room
  auto it = gameRooms.find(game_id);
  
  if (it == gameRooms.end()) {
    response["status"] = "ERROR";
    response["error"] = "Game room not found";
    sendMessage(client_fd, response);
    return;
  }

  // Check if game is full
  if (it->second.players.size() >= MAX_PLAYERS) {
    response["status"] = "ERROR";
    response["error"] = "Game room is full";
    sendMessage(client_fd, response);
    return;
  }
  
  // Validate starting stack
  if(!request.contains("starting_stack") || request["starting_stack"] <= 0) {
    response["status"] = "ERROR";
    response["error"] = "starting_stack must be positive";
    sendMessage(client_fd, response);
    return;
  }

  // Add player to game room
  Player newPlayer;
  newPlayer.username = username;
  newPlayer.server_fd = client_fd;
  newPlayer.gameRoomID = game_id;
  newPlayer.chips = (int)request["starting_stack"];
  newPlayer.hasHand = true;
  newPlayer.currentBet = 0;
  newPlayer.isActive = true;

  // Add to vector and update index map
  int playerIdx = it->second.players.size();
  it->second.players.push_back(newPlayer);
  it->second.playerIndex[username] = playerIdx; 
  
  // Update player-to-game mapping
  playerToGameID[username] = game_id; 
  
  response["status"] = "SUCCESS";
  response["message"] = "Joined game successfully";
  response["player_count"] = it->second.players.size();

  sendMessage(client_fd, response);
  cout << username << " joined game " << game_id << endl;
}

void Server::handleExitGame(const json &request, int client_fd) 
{
  std::lock_guard<std::mutex> lock(server_mutex);
  
  string username = request["username"];
  
  json response = {{"type", "EXIT_GAME_RESPONSE"}};
  
  auto playerGameIt = playerToGameID.find(username);
  
  if (playerGameIt == playerToGameID.end()) {
    response["status"] = "ERROR";
    response["error"] = "Player not in any game";
    sendMessage(client_fd, response);
    return;
  }
  
  int game_id = playerGameIt->second;
  auto roomIt = gameRooms.find(game_id); 
  
  if (roomIt == gameRooms.end()) {
    playerToGameID.erase(username);
    response["status"] = "ERROR";
    response["error"] = "Game room not found";
    sendMessage(client_fd, response);
    return;
  }
  
  GameRoom& room = roomIt->second;
  auto idIt = room.playerIndex.find(username);

  if(idIt == room.playerIndex.end()) {
    playerToGameID.erase(username);
    response["status"] = "ERROR";
    response["error"] = "Player not found in game room";
    sendMessage(client_fd, response);
    return;
  }
  
  int playerIdx = idIt->second;
  
  // Fix: Save the username of the player being moved BEFORE modifying the vector
  if (playerIdx < room.players.size() - 1) {
    std::string movedPlayerName = room.players.back().username;
    room.players[playerIdx] = room.players.back();
    room.playerIndex[movedPlayerName] = playerIdx;
  }
  room.players.pop_back();
  
  room.playerIndex.erase(username);
  playerToGameID.erase(username); 
  
  // If room is empty, delete it and recycle the ID
  if (room.players.empty()) {
    freeGameID.push_back(game_id);
    gameRooms.erase(game_id);
  }
  
  response["status"] = "SUCCESS";
  response["message"] = "Exited game successfully";
  
  sendMessage(client_fd, response);
  cout << username << " exited game " << game_id << endl;
}

void Server::handleUnregister(const json &request, int client_fd, std::string &client_username)
{
  std::lock_guard<std::mutex> lock(server_mutex);
  
  string username = request["username"];
  
  json response = {{"type", "UNREGISTER_RESPONSE"}};
  
  auto it = registeredPlayers.find(username);
  
  if (it == registeredPlayers.end()) {
    response["status"] = "ERROR";
    response["error"] = "Player not registered";
    sendMessage(client_fd, response);
    return;
  }
  
  // Check if player is in a game
  if (playerToGameID.find(username) != playerToGameID.end()) {
    response["status"] = "ERROR";
    response["error"] = "Please exit game before unregistering";
    sendMessage(client_fd, response);
    return;
  }
  
  // Remove player
  registeredPlayers.erase(it);
  client_username = "";
  
  response["status"] = "SUCCESS";
  response["message"] = "Unregistered successfully";
  
  sendMessage(client_fd, response);
  cout << username << " unregistered successfully" << endl;
}

bool Server::handleAction(const json& request, int client_fd) {
  std::lock_guard<std::mutex> lock(server_mutex);
  
  string username = request["username"];
  int game_id = request["game_id"];
  string actionStr = request["action"];
  int amount = request.value("amount", 0); 

  json response = {{"type", "PLAY_TURN_RESPONSE"}};

  // Check if player is in the game
  auto playerInGame = playerToGameID.find(username);
  if(playerInGame == playerToGameID.end() || playerInGame->second != game_id) {
    response["status"] = "ERROR";
    response["error"] = "Player not in session";
    sendMessage(client_fd, response);
    return false;
  }

  // Check for negative amount
  if(amount < 0) {
    response["status"] = "ERROR";
    response["error"] = "Cannot bet a negative amount";
    sendMessage(client_fd, response);
    return false;
  }

  // Find game room
  auto roomIt = gameRooms.find(game_id);
  if (roomIt == gameRooms.end()) {
    response["status"] = "ERROR";
    response["error"] = "Game not found";
    sendMessage(client_fd, response);
    return false;
  }
  
  GameRoom& room = roomIt->second;
  
  // Get player index
  auto playerIdxIt = room.playerIndex.find(username);
  if (playerIdxIt == room.playerIndex.end()) {
    response["status"] = "ERROR";
    response["error"] = "Player not found in game";
    sendMessage(client_fd, response);
    return false;
  }
  
  int playerIdx = playerIdxIt->second;
  Player& player = room.players[playerIdx];

  // Parse action type
  ActionType action = getActionType(actionStr);
  if (action == ActionType::UNKNOWN) {
    response["status"] = "ERROR";
    response["error"] = "Invalid action";
    sendMessage(client_fd, response);
    return false;
  }

  // Handle each action type
  switch(action) {
    case ActionType::FOLD:
      player.hasHand = false;
      player.isActive = false;
      response["status"] = "SUCCESS";
      response["message"] = username + " folded";
      break;
      
    case ActionType::CHECK:
      // Can only check if current bet is 0 or player has already matched it
      if(room.currentBet != player.currentBet) {
        response["status"] = "ERROR";
        response["error"] = "Cannot check, must call or fold";
        sendMessage(client_fd, response);
        return false;
      }
      response["status"] = "SUCCESS";
      response["message"] = username + " checked";
      break;
      
    case ActionType::BET:
      // Can only bet if no one has bet yet
      if(room.currentBet != 0) {
        response["status"] = "ERROR";
        response["error"] = "Cannot bet, someone already bet. Use raise instead";
        sendMessage(client_fd, response);
        return false;
      }
      
      // Bet must be at least big blind
      if(amount < room.bigBlind) {
        response["status"] = "ERROR";
        response["error"] = "Bet must be at least big blind";
        sendMessage(client_fd, response);
        return false;
      }
      
      // Check if player has enough chips
      if(amount > player.chips) {
        response["status"] = "ERROR";
        response["error"] = "Insufficient chips";
        sendMessage(client_fd, response);
        return false;
      }
      
      room.Pot += amount; 
      player.chips -= amount; 
      player.currentBet += amount;
      room.currentBet = amount; 
      response["status"] = "SUCCESS";
      response["message"] = username + " bet " + std::to_string(amount);
      break;
      
    case ActionType::ALL_IN:
      // Player bets all their chips
      amount = player.chips;
      room.Pot += amount;
      player.chips = 0;
      player.currentBet += amount;
      if(amount > room.currentBet) {
        room.currentBet = amount;
      }
      response["status"] = "SUCCESS";
      response["message"] = username + " went all in with " + std::to_string(amount);
      break;
      
    case ActionType::RAISE:
      // Must raise to at least double the current bet
      if(amount < room.currentBet * 2) {
        response["status"] = "ERROR";
        response["error"] = "Raise must be at least double current bet";
        sendMessage(client_fd, response);
        return false;
      }
      
      // Calculate how much more player needs to put in
      int raiseAmount = amount - player.currentBet;
      
      // Check if player has enough chips
      if(raiseAmount > player.chips) {
        response["status"] = "ERROR";
        response["error"] = "Insufficient chips";
        sendMessage(client_fd, response);
        return false;
      }
      
      room.Pot += raiseAmount;
      player.chips -= raiseAmount; 
      player.currentBet = amount;
      room.currentBet = amount; 
      response["status"] = "SUCCESS";
      response["message"] = username + " raised to " + std::to_string(amount);
      break;
      
    case ActionType::CALL:
      // Calculate call amount
      int callAmount = room.currentBet - player.currentBet;
      
      // Validate the amount provided matches what's needed
      if(amount != callAmount) {
        response["status"] = "ERROR";
        response["error"] = "Call amount must be " + std::to_string(callAmount);
        sendMessage(client_fd, response);
        return false;
      }
      
      // Check if player has enough chips
      if(callAmount > player.chips) {
        response["status"] = "ERROR";
        response["error"] = "Insufficient chips to call";
        sendMessage(client_fd, response);
        return false;
      }
      
      room.Pot += callAmount; 
      player.chips -= callAmount;
      player.currentBet += callAmount;
      response["status"] = "SUCCESS";
      response["message"] = username + " called " + std::to_string(callAmount);
      break;
      
    default: 
      response["status"] = "ERROR";
      response["error"] = "Unknown action type";  
      sendMessage(client_fd, response);
      return false; 
  }
  
  sendMessage(client_fd, response);
  cout << "Action processed: " << username << " - " << actionStr << endl;
  return true; 
}