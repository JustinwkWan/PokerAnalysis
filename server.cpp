#include "server.h"
#include "poker.h"
#include <thread>
#include <mutex>
#include <algorithm>
#include <cstring>
#include <openssl/sha.h>
#include <libkern/OSByteOrder.h>
#include <poll.h>

// Global mutex for thread safety
std::mutex server_mutex;

// WebSocket constants
static const std::string WS_MAGIC_STRING = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

Server::Server()
{
    nextGameID = 1;
    
    // Create TCP socket
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

    // Setup address for TCP (port 8080)
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // Bind TCP socket
    if (bind(server_fd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        cerr << "TCP Bind failed" << endl;
        exit(EXIT_FAILURE);
    }

    // Listen on TCP
    if (listen(server_fd, 10) < 0)
    {
        cerr << "TCP Listen failed" << endl;
        exit(EXIT_FAILURE);
    }

    cout << "TCP Server started on port 8080" << endl;
    
    // Create WebSocket server socket (port 8081)
    ws_server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (ws_server_fd == 0)
    {
        cerr << "WebSocket socket creation error" << endl;
        exit(EXIT_FAILURE);
    }

    if (setsockopt(ws_server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                   &opt, sizeof(opt)))
    {
        cerr << "WebSocket setsockopt error" << endl;
        exit(EXIT_FAILURE);
    }

    sockaddr_in wsAddress;
    wsAddress.sin_family = AF_INET;
    wsAddress.sin_port = htons(8081);
    wsAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(ws_server_fd, (struct sockaddr *)&wsAddress, sizeof(wsAddress)) < 0)
    {
        cerr << "WebSocket Bind failed" << endl;
        exit(EXIT_FAILURE);
    }

    if (listen(ws_server_fd, 10) < 0)
    {
        cerr << "WebSocket Listen failed" << endl;
        exit(EXIT_FAILURE);
    }

    cout << "WebSocket Server started on port 8081" << endl;
}

Server::~Server()
{
    close(server_fd);
    close(ws_server_fd);
}

std::string Server::base64Encode(const unsigned char* data, size_t len) {
    static const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    
    for (size_t i = 0; i < len; i += 3) {
        int b = (data[i] & 0xFC) >> 2;
        result += chars[b];
        
        b = (data[i] & 0x03) << 4;
        if (i + 1 < len) {
            b |= (data[i + 1] & 0xF0) >> 4;
            result += chars[b];
            b = (data[i + 1] & 0x0F) << 2;
            if (i + 2 < len) {
                b |= (data[i + 2] & 0xC0) >> 6;
                result += chars[b];
                b = data[i + 2] & 0x3F;
                result += chars[b];
            } else {
                result += chars[b];
                result += '=';
            }
        } else {
            result += chars[b];
            result += "==";
        }
    }
    
    return result;
}

bool Server::performWebSocketHandshake(int client_fd) {
    char buffer[4096];
    ssize_t received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (received <= 0) return false;
    buffer[received] = '\0';
    
    std::string request(buffer);
    
    // Find Sec-WebSocket-Key header (try different capitalizations)
    std::string key_header = "Sec-WebSocket-Key: ";
    size_t key_pos = request.find(key_header);
    
    if (key_pos == std::string::npos) {
        key_header = "Sec-Websocket-Key: ";
        key_pos = request.find(key_header);
    }
    if (key_pos == std::string::npos) {
        key_header = "sec-websocket-key: ";
        key_pos = request.find(key_header);
    }
    
    if (key_pos == std::string::npos) return false;
    
    size_t key_start = key_pos + key_header.length();
    size_t key_end = request.find("\r\n", key_start);
    std::string key = request.substr(key_start, key_end - key_start);
    
    // Generate accept key
    std::string accept_key_input = key + WS_MAGIC_STRING;
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(accept_key_input.c_str()), 
         accept_key_input.length(), hash);
    std::string accept_key = base64Encode(hash, SHA_DIGEST_LENGTH);
    
    // Send handshake response
    std::string response = 
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: " + accept_key + "\r\n\r\n";
    
    send(client_fd, response.c_str(), response.length(), 0);
    return true;
}

json Server::receiveWebSocketMessage(int client_fd) {
    unsigned char header[2];
    ssize_t received = recv(client_fd, header, 2, MSG_WAITALL);
    if (received <= 0) throw std::runtime_error("Client disconnected");
  
    int opcode = header[0] & 0x0F;
    bool masked = (header[1] & 0x80) != 0;
    uint64_t payload_len = header[1] & 0x7F;
    
    // Handle close frame
    if (opcode == 8) {
        throw std::runtime_error("Client closed connection");
    }
    
    // Handle ping frame
    if (opcode == 9) {
        // Send pong
        unsigned char pong[2] = {0x8A, 0x00};
        send(client_fd, pong, 2, 0);
        return receiveWebSocketMessage(client_fd);  // Continue waiting for real message
    }
    
    // Get extended payload length
    if (payload_len == 126) {
        uint16_t len16;
        recv(client_fd, &len16, 2, MSG_WAITALL);
        payload_len = ntohs(len16);
    } else if (payload_len == 127) {
        uint64_t len64;
        recv(client_fd, &len64, 8, MSG_WAITALL);
        #ifdef __APPLE__
payload_len = OSSwapBigToHostInt64(len64);
#else
payload_len = be64toh(len64);
#endif
    }
    
    // Get mask key if present
    unsigned char mask[4] = {0};
    if (masked) {
        recv(client_fd, mask, 4, MSG_WAITALL);
    }
    
    // Get payload
    std::vector<char> payload(payload_len);
    if (payload_len > 0) {
        received = recv(client_fd, payload.data(), payload_len, MSG_WAITALL);
        if (received <= 0) throw std::runtime_error("Client disconnected");
        
        // Unmask data
        if (masked) {
            for (size_t i = 0; i < payload_len; i++) {
                payload[i] ^= mask[i % 4];
            }
        }
    }
    
    std::string json_str(payload.begin(), payload.end());
    return json::parse(json_str);
}

void Server::sendWebSocketMessage(int client_fd, const json& message) {
    std::string payload = message.dump();
    std::vector<unsigned char> frame;
    
    cout << "Sending WebSocket message, payload length: " << payload.length() << endl;
    
    frame.push_back(0x81);
    
    // Payload length (server doesn't mask)
    if (payload.length() < 126) {
        frame.push_back(static_cast<unsigned char>(payload.length()));
        cout << "Using short length format" << endl;
    } else if (payload.length() < 65536) {
        frame.push_back(126);
        frame.push_back((payload.length() >> 8) & 0xFF);
        frame.push_back(payload.length() & 0xFF);
        cout << "Using 16-bit length format" << endl;
    } else {
        frame.push_back(127);
        uint64_t len = payload.length();
        for (int i = 7; i >= 0; i--) {
            frame.push_back((len >> (i * 8)) & 0xFF);
        }
        cout << "Using 64-bit length format" << endl;
    }
    
    // Payload
    frame.insert(frame.end(), payload.begin(), payload.end());
    
    ssize_t sent = send(client_fd, frame.data(), frame.size(), 0);
    cout << "Sent " << sent << " bytes (frame size: " << frame.size() << ")" << endl;
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
    auto it = clientTypes.find(clientSocket);
    if (it != clientTypes.end() && it->second == ClientType::WEBSOCKET) {
        return receiveWebSocketMessage(clientSocket);
    }
    
    // TCP message
    uint32_t msg_length_net;
    ssize_t received = recv(clientSocket, &msg_length_net, sizeof(msg_length_net), MSG_WAITALL);
    if (received <= 0)
    {
        throw std::runtime_error("Client disconnected");
    }

    uint32_t msg_length = ntohl(msg_length_net);
    
    if (msg_length > 1000000) {
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
    auto it = clientTypes.find(clientSocket);
    if (it != clientTypes.end() && it->second == ClientType::WEBSOCKET) {
        sendWebSocketMessage(clientSocket, message);
        return;
    }
    
    // TCP message
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
    // Accept connections on both TCP and WebSocket ports
    std::thread tcp_thread([this]() {
        while (true) {
            sockaddr_in clientAddress;
            socklen_t clientLen = sizeof(clientAddress);
            int client_fd = accept(server_fd, (struct sockaddr *)&clientAddress, &clientLen);
            
            if (client_fd < 0) {
                cerr << "TCP Accept failed" << endl;
                continue;
            }

            cout << "New TCP client connected: fd=" << client_fd << endl;
            {
                std::lock_guard<std::mutex> lock(server_mutex);
                clientTypes[client_fd] = ClientType::TCP;
            }
            std::thread(&Server::handleClient, this, client_fd, ClientType::TCP).detach();
        }
    });
    
    // WebSocket acceptor (main thread)
    while (true) {
        sockaddr_in clientAddress;
        socklen_t clientLen = sizeof(clientAddress);
        int client_fd = accept(ws_server_fd, (struct sockaddr *)&clientAddress, &clientLen);
        
        if (client_fd < 0) {
            cerr << "WebSocket Accept failed" << endl;
            continue;
        }

        cout << "New WebSocket client connecting: fd=" << client_fd << endl;
        
        // Perform WebSocket handshake
        if (!performWebSocketHandshake(client_fd)) {
            cerr << "WebSocket handshake failed" << endl;
            close(client_fd);
            continue;
        }
        
        cout << "WebSocket handshake successful: fd=" << client_fd << endl;
        {
            std::lock_guard<std::mutex> lock(server_mutex);
            clientTypes[client_fd] = ClientType::WEBSOCKET;
        }
        std::thread(&Server::handleClient, this, client_fd, ClientType::WEBSOCKET).detach();
    }
}

void Server::handleClient(int client_fd, ClientType /*clientType*/)
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
            case MessageType::START_GAME:
                handleStartGame(request, client_fd);
                break;
            case MessageType::PLAY_TURN:
                handleAction(request, client_fd);
                break;
            default:
                cout << "Unknown message type received." << endl;
                json response = {
                    {"type", "ERROR"},
                    {"error", "Unknown message type"}
                };
                sendMessage(client_fd, response);
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
                        
                        if (playerIdx < (int)room.players.size() - 1) {
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
        
        clientTypes.erase(client_fd);
        close(client_fd);
    }
}

void Server::handleRegister(const json &request, int client_fd, std::string &client_username)
{
    std::lock_guard<std::mutex> lock(server_mutex);
    
    string username = request["name"];

    if(username.empty()) {
        json response = {
            {"type", "REGISTER_RESPONSE"},
            {"status", "ERROR"},
            {"error", "empty username"}
        };
        sendMessage(client_fd, response);
        return; 
    }
    
    if (registeredPlayers.find(username) != registeredPlayers.end())
    {
        json response = {
            {"type", "REGISTER_RESPONSE"},
            {"status", "ERROR"},
            {"error", "Username already taken"}};
        sendMessage(client_fd, response);
        return;
    }

    registeredPlayers[username] = client_fd;
    client_username = username;

    json response = {
        {"type", "REGISTER_RESPONSE"},
        {"status", "SUCCESS"},
        {"message", "Welcome " + username}};

    sendMessage(client_fd, response);
}

void Server::handleListGames(const json& /*request*/, int client_fd)
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
        cout << "LIST_GAMES_RESPONSE (empty): " << response.dump() << endl;
        sendMessage(client_fd, response);
        return;
    }

    for (const auto &game : gameRooms)
    {
        response["games"].push_back({
            {"game_id", game.second.gameID},
            {"small_blind", game.second.smallBlind},
            {"big_blind", game.second.bigBlind},
            {"player_count", game.second.players.size()},
            {"game_stage", game.second.gameStages}
        });
    }
    
    cout << "LIST_GAMES_RESPONSE: " << response.dump() << endl;
    sendMessage(client_fd, response);
}

void Server::handleCreateGame(const json &request, int client_fd)
{
    std::lock_guard<std::mutex> lock(server_mutex);
    
    if (!request.contains("small_blind") || !request.contains("big_blind"))
    {
        json response = {
            {"type", "CREATE_GAME_RESPONSE"},
            {"status", "ERROR"},
            {"error", "Missing game parameters"}
        };
        sendMessage(client_fd, response);
        return;
    }
    
    int smallBlind = request["small_blind"];
    int bigBlind = request["big_blind"];
    
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

    int game_id;
    if (!freeGameID.empty()) {
        game_id = freeGameID.back();
        freeGameID.pop_back();
    } else {
        game_id = nextGameID++;
    }

    GameRoom newRoom;
    newRoom.gameID = game_id;
    newRoom.smallBlind = smallBlind;
    newRoom.bigBlind = bigBlind;
    newRoom.currentPlayerIndex = 0;
    newRoom.Pot = 0;
    newRoom.currentBet = 0;
    newRoom.buttonPosition = 0;
    newRoom.gameStages = "WAITING";
    newRoom.lastRaiser = -1;
    newRoom.poker = std::make_unique<Poker>();
    
    gameRooms[game_id] = std::move(newRoom);
    
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

    if (registeredPlayers.find(username) == registeredPlayers.end()) {
        response["status"] = "ERROR";
        response["error"] = "Invalid Player ID";
        sendMessage(client_fd, response);
        return;
    }
    
    if (playerToGameID.find(username) != playerToGameID.end()) {
        response["status"] = "ERROR";
        response["error"] = "Player already in a game";
        sendMessage(client_fd, response);
        return;
    }
    
    auto it = gameRooms.find(game_id);
    
    if (it == gameRooms.end()) {
        response["status"] = "ERROR";
        response["error"] = "Game room not found";
        sendMessage(client_fd, response);
        return;
    }

    if (it->second.players.size() >= MAX_PLAYERS) {
        response["status"] = "ERROR";
        response["error"] = "Game room is full";
        sendMessage(client_fd, response);
        return;
    }
    
    if (it->second.gameStages != "WAITING") {
        response["status"] = "ERROR";
        response["error"] = "Game already in progress";
        sendMessage(client_fd, response);
        return;
    }
    
    if(!request.contains("starting_stack") || request["starting_stack"] <= 0) {
        response["status"] = "ERROR";
        response["error"] = "starting_stack must be positive";
        sendMessage(client_fd, response);
        return;
    }

    Player newPlayer; 
    newPlayer.username = username;
    newPlayer.server_fd = client_fd;
    newPlayer.gameRoomID = game_id;
    newPlayer.chips = (int)request["starting_stack"];
    newPlayer.hasHand = true;
    newPlayer.currentBet = 0;
    newPlayer.isActive = true;
    
    auto typeIt = clientTypes.find(client_fd);
    newPlayer.clientType = (typeIt != clientTypes.end()) ? typeIt->second : ClientType::TCP;

    int playerIdx = it->second.players.size();
    it->second.players.push_back(newPlayer);
    it->second.playerIndex[username] = playerIdx; 
    
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
    
    if (playerIdx < (int)room.players.size() - 1) {
        std::string movedPlayerName = room.players.back().username;
        room.players[playerIdx] = room.players.back();
        room.playerIndex[movedPlayerName] = playerIdx;
    }
    room.players.pop_back();
    
    room.playerIndex.erase(username);
    playerToGameID.erase(username); 
    
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
    
    if (playerToGameID.find(username) != playerToGameID.end()) {
        response["status"] = "ERROR";
        response["error"] = "Please exit game before unregistering";
        sendMessage(client_fd, response);
        return;
    }
    
    registeredPlayers.erase(it);
    client_username = "";
    
    response["status"] = "SUCCESS";
    response["message"] = "Unregistered successfully";
    
    sendMessage(client_fd, response);
    cout << username << " unregistered successfully" << endl;
}

void Server::handleStartGame(const json& request, int client_fd) {
    std::lock_guard<std::mutex> lock(server_mutex);
    
    int game_id = request["game_id"];
    string username = request["username"];
    
    json response = {{"type", "START_GAME_RESPONSE"}};
    
    auto roomIt = gameRooms.find(game_id);
    if (roomIt == gameRooms.end()) {
        response["status"] = "ERROR";
        response["error"] = "Game not found";
        sendMessage(client_fd, response);
        return;
    }
    
    GameRoom& room = roomIt->second;
    
    if (room.players.size() < 2) {
        response["status"] = "ERROR";
        response["error"] = "Need at least 2 players to start";
        sendMessage(client_fd, response);
        return;
    }
    
    if (room.gameStages != "WAITING") {
        response["status"] = "ERROR";
        response["error"] = "Game already started";
        sendMessage(client_fd, response);
        return;
    }
    
    // Start the game
    room.poker->resetDeck();
    room.gameStages = "PREFLOP";
    room.Pot = 0;
    room.currentBet = 0;
    room.hasActedThisRound.clear();
    room.lastRaiser = -1;
    
    // Deal hole cards to each player
    for (auto& player : room.players) {
        player.holeCards = room.poker->dealCards(2);
        player.hasHand = true;
        player.isActive = true;
        player.currentBet = 0;
    }
    
    // Post blinds
    int numPlayers = room.players.size();
    int sbIdx, bbIdx;
    
    if (numPlayers == 2) {
        // Heads up: button posts small blind
        sbIdx = room.buttonPosition;
        bbIdx = (room.buttonPosition + 1) % numPlayers;
    } else {
        sbIdx = (room.buttonPosition + 1) % numPlayers;
        bbIdx = (room.buttonPosition + 2) % numPlayers;
    }
    
    int sbAmount = std::min(room.smallBlind, room.players[sbIdx].chips);
    room.players[sbIdx].chips -= sbAmount;
    room.players[sbIdx].currentBet = sbAmount;
    room.Pot += sbAmount;
    
    int bbAmount = std::min(room.bigBlind, room.players[bbIdx].chips);
    room.players[bbIdx].chips -= bbAmount;
    room.players[bbIdx].currentBet = bbAmount;
    room.Pot += bbAmount;
    room.currentBet = bbAmount;
    
    // Set current player (first to act after big blind)
    room.currentPlayerIndex = (bbIdx + 1) % numPlayers;
    
    // Mark blinds as having acted (they can still raise though)
    room.lastRaiser = bbIdx;  // BB is considered the last "raiser" preflop
    
    response["status"] = "SUCCESS";
    response["message"] = "Game started";
    response["pot"] = room.Pot;
    response["current_bet"] = room.currentBet;
    
    sendMessage(client_fd, response);
    
    // Notify all players about game start and their hole cards
    for (size_t i = 0; i < room.players.size(); i++) {
        json notify = {
            {"type", "GAME_STARTED"},
            {"game_id", game_id},
            {"your_position", i},
            {"button_position", room.buttonPosition},
            {"small_blind_position", sbIdx},
            {"big_blind_position", bbIdx},
            {"pot", room.Pot},
            {"current_bet", room.currentBet},
            {"current_player", room.currentPlayerIndex},
            {"hole_cards", json::array()},
            {"players", json::array()}
        };
        
        // Add hole cards for this player
        for (const auto& card : room.players[i].holeCards) {
            notify["hole_cards"].push_back({
                {"rank", room.poker->rankToString(card.rank)},
                {"suit", room.poker->suitToString(card.suit)}
            });
        }
        
        // Add all player info
        for (const auto& p : room.players) {
            notify["players"].push_back({
                {"username", p.username},
                {"chips", p.chips},
                {"current_bet", p.currentBet},
                {"is_active", p.isActive},
                {"has_hand", p.hasHand}
            });
        }
        
        sendMessage(room.players[i].server_fd, notify);
    }
    
    cout << "Game " << game_id << " started with " << room.players.size() << " players" << endl;
}

bool Server::isBettingRoundComplete(GameRoom& room) {
    int activePlayers = 0;
    int playersToAct = 0;
    
    for (size_t i = 0; i < room.players.size(); i++) {
        const auto& p = room.players[i];
        if (p.hasHand && p.isActive) {
            activePlayers++;
            // Player needs to act if they haven't matched current bet (and have chips)
            // OR if they haven't acted since the last raise
            if (p.currentBet < room.currentBet && p.chips > 0) {
                playersToAct++;
            } else if (room.hasActedThisRound.find(p.username) == room.hasActedThisRound.end()) {
                playersToAct++;
            }
        }
    }
    
    // Round is complete when everyone has acted and bets are matched
    return playersToAct == 0 && activePlayers > 0;
}

void Server::advanceGameStage(GameRoom& room) {
    // Reset for new betting round
    room.currentBet = 0;
    room.hasActedThisRound.clear();
    room.lastRaiser = -1;
    
    for (auto& player : room.players) {
        player.currentBet = 0;
    }
    
    if (room.gameStages == "PREFLOP") {
        room.poker->dealFlop();
        room.gameStages = "FLOP";
    } else if (room.gameStages == "FLOP") {
        room.poker->dealTurn();
        room.gameStages = "TURN";
    } else if (room.gameStages == "TURN") {
        room.poker->dealRiver();
        room.gameStages = "RIVER";
    } else if (room.gameStages == "RIVER") {
        room.gameStages = "SHOWDOWN";
        handleShowdown(room);
        return;
    }
    
    // Set first to act (first active player after button)
    int numPlayers = room.players.size();
    for (int i = 1; i <= numPlayers; i++) {
        int idx = (room.buttonPosition + i) % numPlayers;
        if (room.players[idx].hasHand && room.players[idx].isActive && room.players[idx].chips > 0) {
            room.currentPlayerIndex = idx;
            break;
        }
    }
    
    // Broadcast new community cards to all players
    broadcastGameState(room);
}

void Server::handleShowdown(GameRoom& room) {
    // Collect all active players' hands
    std::vector<std::vector<Card>> activeHands;
    std::vector<int> activePlayerIndices;
    
    for (size_t i = 0; i < room.players.size(); i++) {
        if (room.players[i].hasHand && room.players[i].isActive) {
            activeHands.push_back(room.players[i].holeCards);
            activePlayerIndices.push_back(i);
        }
    }
    
    // Determine winners
    std::vector<int> winnerIndices = room.poker->determineWinners(activeHands);
    
    // Convert to actual player indices
    std::vector<int> actualWinners;
    for (int idx : winnerIndices) {
        actualWinners.push_back(activePlayerIndices[idx]);
    }
    
    // Split pot among winners
    int potShare = room.Pot / actualWinners.size();
    int remainder = room.Pot % actualWinners.size();
    
    for (size_t i = 0; i < actualWinners.size(); i++) {
        int winnerIdx = actualWinners[i];
        room.players[winnerIdx].chips += potShare;
        if ((int)i == 0) {
            room.players[winnerIdx].chips += remainder;
        }
    }
    
    // Broadcast showdown results
    json showdownMsg = {
        {"type", "SHOWDOWN"},
        {"game_id", room.gameID},
        {"pot", room.Pot},
        {"winners", json::array()},
        {"community_cards", json::array()},
        {"all_hands", json::array()}
    };
    
    // Add winner info
    for (size_t i = 0; i < actualWinners.size(); i++) {
        int winnerIdx = actualWinners[i];
        const Player& winner = room.players[winnerIdx];
        Poker::HandValue handVal = room.poker->evaluateHandDetailed(winner.holeCards);
        
        showdownMsg["winners"].push_back({
            {"username", winner.username},
            {"hand_rank", room.poker->handRankToString(handVal.rank)},
            {"chips_won", potShare + ((int)i == 0 ? remainder : 0)},
            {"total_chips", winner.chips}
        });
    }
    
    // Add all hands for display
    for (size_t i = 0; i < room.players.size(); i++) {
        const auto& p = room.players[i];
        if (p.hasHand) {
            json playerHand = {
                {"username", p.username},
                {"cards", json::array()}
            };
            for (const auto& card : p.holeCards) {
                playerHand["cards"].push_back({
                    {"rank", room.poker->rankToString(card.rank)},
                    {"suit", room.poker->suitToString(card.suit)}
                });
            }
            Poker::HandValue hv = room.poker->evaluateHandDetailed(p.holeCards);
            playerHand["hand_rank"] = room.poker->handRankToString(hv.rank);
            showdownMsg["all_hands"].push_back(playerHand);
        }
    }
    
    // Add community cards
    for (const auto& card : room.poker->getCommunityCards()) {
        showdownMsg["community_cards"].push_back({
            {"rank", room.poker->rankToString(card.rank)},
            {"suit", room.poker->suitToString(card.suit)}
        });
    }
    
    // Add final player states
    showdownMsg["players"] = json::array();
    for (const auto& p : room.players) {
        showdownMsg["players"].push_back({
            {"username", p.username},
            {"chips", p.chips}
        });
    }
    
    // Send to all players
    for (const auto& player : room.players) {
        sendMessage(player.server_fd, showdownMsg);
    }
    
    // Reset for next hand
    room.gameStages = "WAITING";
    room.Pot = 0;
    room.currentBet = 0;
    room.buttonPosition = (room.buttonPosition + 1) % room.players.size();
    room.hasActedThisRound.clear();
    room.lastRaiser = -1;
    
    cout << "Showdown complete for game " << room.gameID << endl;
}

void Server::broadcastGameState(GameRoom& room) {
    std::vector<Card> communityCards = room.poker->getCommunityCards();
    
    json stateMsg = {
        {"type", "GAME_STATE_UPDATE"},
        {"game_id", room.gameID},
        {"stage", room.gameStages},
        {"pot", room.Pot},
        {"current_bet", room.currentBet},
        {"current_player", room.currentPlayerIndex},
        {"current_player_name", room.players[room.currentPlayerIndex].username},
        {"community_cards", json::array()},
        {"players", json::array()}
    };
    
    // Add community cards
    for (const auto& card : communityCards) {
        stateMsg["community_cards"].push_back({
            {"rank", room.poker->rankToString(card.rank)},
            {"suit", room.poker->suitToString(card.suit)}
        });
    }
    
    // Add player info (without hole cards)
    for (const auto& player : room.players) {
        stateMsg["players"].push_back({
            {"username", player.username},
            {"chips", player.chips},
            {"current_bet", player.currentBet},
            {"is_active", player.isActive},
            {"has_hand", player.hasHand}
        });
    }
    
    // Send to all players
    for (const auto& player : room.players) {
        sendMessage(player.server_fd, stateMsg);
    }
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
    
    // Check game is in progress
    if (room.gameStages == "WAITING" || room.gameStages == "SHOWDOWN") {
        response["status"] = "ERROR";
        response["error"] = "Game not in progress";
        sendMessage(client_fd, response);
        return false;
    }
    
    // Check if it's player's turn
    auto playerIdxIt = room.playerIndex.find(username);
    if (playerIdxIt == room.playerIndex.end()) {
        response["status"] = "ERROR";
        response["error"] = "Player not found in game";
        sendMessage(client_fd, response);
        return false;
    }
    
    int playerIdx = playerIdxIt->second;
    
    if (playerIdx != room.currentPlayerIndex) {
        response["status"] = "ERROR";
        response["error"] = "Not your turn";
        sendMessage(client_fd, response);
        return false;
    }
    
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
        case ActionType::FOLD: {
            player.hasHand = false;
            player.isActive = false;
            response["status"] = "SUCCESS";
            response["message"] = username + " folded";
            break;
        }
        case ActionType::CHECK: {
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
        }
        case ActionType::BET: {
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
                response["error"] = "Bet must be at least big blind (" + std::to_string(room.bigBlind) + ")";
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
            room.currentBet = player.currentBet;
            room.lastRaiser = playerIdx;
            room.hasActedThisRound.clear();  // Reset - everyone needs to act again
            response["status"] = "SUCCESS";
            response["message"] = username + " bet " + std::to_string(amount);
            break;
        }
        case ActionType::ALL_IN: {
            // Player bets all their chips
            int allInAmount = player.chips;
            room.Pot += allInAmount;
            player.currentBet += allInAmount;
            player.chips = 0;
            player.isActive = false;  // Can't act anymore but still has hand
            
            if(player.currentBet > room.currentBet) {
                room.currentBet = player.currentBet;
                room.lastRaiser = playerIdx;
                room.hasActedThisRound.clear();
            }
            response["status"] = "SUCCESS";
            response["message"] = username + " went all in with " + std::to_string(allInAmount);
            break;
        }
        case ActionType::RAISE: {
            // Raise amount is the TOTAL bet, not the additional amount
            if(room.currentBet == 0) {
                response["status"] = "ERROR";
                response["error"] = "No bet to raise, use BET instead";
                sendMessage(client_fd, response);
                return false;
            }
            
            int minRaise = room.currentBet * 2;
            if(amount < minRaise) {
                response["status"] = "ERROR";
                response["error"] = "Raise must be at least " + std::to_string(minRaise);
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
            room.lastRaiser = playerIdx;
            room.hasActedThisRound.clear();  // Everyone needs to act again
            response["status"] = "SUCCESS";
            response["message"] = username + " raised to " + std::to_string(amount);
            break;
        }
        case ActionType::CALL: {
            // Calculate call amount
            int callAmount = room.currentBet - player.currentBet;
            
            if(callAmount <= 0) {
                response["status"] = "ERROR";
                response["error"] = "Nothing to call";
                sendMessage(client_fd, response);
                return false;
            }
            
            // Check if player has enough chips (if not, it's an all-in)
            if(callAmount > player.chips) {
                callAmount = player.chips;
                player.isActive = false;  // All-in
            }
            
            room.Pot += callAmount; 
            player.chips -= callAmount;
            player.currentBet += callAmount;
            response["status"] = "SUCCESS";
            response["message"] = username + " called " + std::to_string(callAmount);
            break;
        }
        default: {
            response["status"] = "ERROR";
            response["error"] = "Unknown action type";  
            sendMessage(client_fd, response);
            return false;
        }
    }
    
    // Mark player as having acted
    room.hasActedThisRound[username] = true;
    
    sendMessage(client_fd, response);
    
    // Count active players
    int activePlayers = 0;
    int playersWithHand = 0;
    for (const auto& p : room.players) {
        if (p.hasHand) {
            playersWithHand++;
            if (p.isActive && p.chips > 0) {
                activePlayers++;
            }
        }
    }
    
    // If only one player left with a hand, they win
    if (playersWithHand == 1) {
        for (auto& p : room.players) {
            if (p.hasHand) {
                p.chips += room.Pot;
                
                json winMsg = {
                    {"type", "GAME_OVER"},
                    {"game_id", game_id},
                    {"winner", p.username},
                    {"pot", room.Pot},
                    {"reason", "All other players folded"},
                    {"players", json::array()}
                };
                
                for (const auto& pl : room.players) {
                    winMsg["players"].push_back({
                        {"username", pl.username},
                        {"chips", pl.chips}
                    });
                }
                
                for (const auto& player : room.players) {
                    sendMessage(player.server_fd, winMsg);
                }
                
                room.gameStages = "WAITING";
                room.Pot = 0;
                room.currentBet = 0;
                room.buttonPosition = (room.buttonPosition + 1) % room.players.size();
                room.hasActedThisRound.clear();
                break;
            }
        }
        return true;
    }
    
    // Move to next active player
    int startIdx = room.currentPlayerIndex;
    do {
        room.currentPlayerIndex = (room.currentPlayerIndex + 1) % room.players.size();
        
        // Prevent infinite loop
        if (room.currentPlayerIndex == startIdx) {
            break;
        }
    } while (!room.players[room.currentPlayerIndex].hasHand || 
             !room.players[room.currentPlayerIndex].isActive ||
             room.players[room.currentPlayerIndex].chips == 0);
    
    // Check if betting round is complete
    if (isBettingRoundComplete(room) || activePlayers <= 1) {
        // If everyone is all-in or only one active player, deal remaining cards
        if (activePlayers <= 1 && playersWithHand > 1) {
            // Run out the board
            while (room.gameStages != "RIVER" && room.gameStages != "SHOWDOWN") {
                if (room.gameStages == "PREFLOP") {
                    room.poker->dealFlop();
                    room.gameStages = "FLOP";
                } else if (room.gameStages == "FLOP") {
                    room.poker->dealTurn();
                    room.gameStages = "TURN";
                } else if (room.gameStages == "TURN") {
                    room.poker->dealRiver();
                    room.gameStages = "RIVER";
                }
            }
            room.gameStages = "SHOWDOWN";
            handleShowdown(room);
        } else {
            advanceGameStage(room);
        }
    } else {
        // Broadcast game state
        broadcastGameState(room);
    }
    
    cout << "Action processed: " << username << " - " << actionStr << endl;
    return true; 
}