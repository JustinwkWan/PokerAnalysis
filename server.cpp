#include "server.h"

Server::Server() {
  // Create socket
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == 0) {
    cerr << "Socket creation error" << endl;
    exit(EXIT_FAILURE);
  }
  
  // Set socket options
  int opt = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                 &opt, sizeof(opt))) {
    cerr << "setsockopt error" << endl;
    exit(EXIT_FAILURE);
  }
  
  // Setup address
  sockaddr_in serverAddress;
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(8080);
  serverAddress.sin_addr.s_addr = INADDR_ANY;
  
  // Bind socket
  if (bind(server_fd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
    cerr << "Bind failed" << endl;
    exit(EXIT_FAILURE);
  }
  
  // Listen
  if (listen(server_fd, 5) < 0) {
    cerr << "Listen failed" << endl;
    exit(EXIT_FAILURE);
  }

  // Accept connection
  client_fd = accept(server_fd, nullptr, nullptr);
  if (client_fd < 0) {
    cerr << "Accept failed" << endl;
    exit(EXIT_FAILURE);
  }
  
}

Server::~Server() {
  close(client_fd);
  close(server_fd);
}

MessageType Server::getMessageType(const std::string& typeStr) {
  if (typeStr == "LOGIN") return MessageType::LOGIN;
  if (typeStr == "REGISTER") return MessageType::REGISTER;
  if (typeStr == "CREATE_GAME") return MessageType::CREATE_GAME;
  if (typeStr == "JOIN_GAME") return MessageType::JOIN_GAME;
  if (typeStr == "EXIT_GAME") return MessageType::EXIT_GAME;
  if (typeStr == "UNREGISTER") return MessageType::UNREGISTER;
  if (typeStr == "START_GAME") return MessageType::START_GAME;
  if (typeStr == "PLAY_TURN") return MessageType::PLAY_TURN;
  if (typeStr == "LIST_GAMES") return MessageType::LIST_GAMES;
  return MessageType::UNKNOWN;
}
json Server::receiveMessage(int clientSocket) {
  uint32_t msg_length_net;
  ssize_t received = recv(clientSocket, &msg_length_net, sizeof(msg_length_net), MSG_WAITALL);
  if (received <= 0) {
    throw std::runtime_error("Client disconnected");
  }
  
  uint32_t msg_length = ntohl(msg_length_net);
  std::vector<char> buffer(msg_length);
  
  received = recv(clientSocket, buffer.data(), msg_length, MSG_WAITALL);
  if (received <= 0) {
    throw std::runtime_error("Client disconnected");
  }
  
  std::string json_str(buffer.begin(), buffer.end());
  return json::parse(json_str);
}

void Server::sendMessage(int clientSocket, const json& message) {
  std::string jsonString = message.dump();
  uint32_t length = htonl(jsonString.size());
  
  ssize_t bytesSent = send(clientSocket, &length, sizeof(length), 0);
  if (bytesSent != sizeof(length)) {
    throw std::runtime_error("Failed to send message length");
  }
  
  bytesSent = send(clientSocket, jsonString.c_str(), jsonString.length(), 0);
  if (bytesSent != (ssize_t)jsonString.length()) {
    throw std::runtime_error("Failed to send message");
  }
}

void Server::handleClient() {
  try {
    while (true) {
      json request = receiveMessage(client_fd);
      
      cout << "\n=== Received Message ===" << endl;
      cout << request.dump(2) << endl;
      MessageType msgType = getMessageType(request["type"]);

      switch (msgType) {
        case MessageType::REGISTER:
          handleRegister(request);
          break;
        case MessageType::LIST_GAMES:
          handleListGames(request);
          break;
        default:
          cout << "Unknown message type received." << endl;
          break;
      }
    }
  } catch (const std::exception& e) {
    cerr << "Client handling error: " << e.what() << endl;
    close(client_fd);
  }
}


void Server::handleRegister(const json& request) {
  string username = request["name"];
  
  cout << "Register request from: " << username << endl;
  
  // Check if username already taken
  if (registeredPlayers.find(username) != registeredPlayers.end()) {
    json response = {
      {"type", "REGISTER_RESPONSE"},
      {"status", "ERROR"},
      {"error", "Username already taken"}
    };
    sendMessage(client_fd, response);
    cout << "Registration failed: Username taken" << endl;
    return;
  }
  
  // Register the player
  registeredPlayers[username] = client_fd;
  
  json response = {
    {"type", "REGISTER_RESPONSE"},
    {"status", "SUCCESS"},
    {"message", "Welcome " + username}
  };
  
  sendMessage(client_fd, response);
  cout << "Registration successful for: " << username << endl;
  cout << "Total registered players: " << registeredPlayers.size() << endl;
}
void Server::handleListGames(const json& request) {
  
}