#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "json.hpp"
#include "poker.h"

using json = nlohmann::json;
using std::string;
using std::cout;
using std::cerr;
using std::endl;

const int MAX_PLAYERS = 9;

enum class MessageType {
    LOGIN,
    REGISTER,
    CREATE_GAME,
    JOIN_GAME,
    EXIT_GAME,
    UNREGISTER,
    START_GAME,
    PLAY_TURN,
    LIST_GAMES,
    UNKNOWN
};

enum class ActionType {
    FOLD,
    CHECK,
    CALL,
    BET,
    RAISE,
    ALL_IN,
    UNKNOWN
};

enum class ClientType {
    TCP,
    WEBSOCKET
};

struct Player {
    string username;
    int server_fd;
    int gameRoomID;
    int chips;
    std::vector<Card> holeCards;
    bool hasHand;
    int currentBet;
    bool isActive;
    ClientType clientType;
};

struct GameRoom {
    int gameID;
    int smallBlind;
    int bigBlind;
    std::vector<Player> players;
    std::unordered_map<string, int> playerIndex;
    int currentPlayerIndex;
    int Pot;
    int currentBet;
    int buttonPosition;
    string gameStages;
    std::unordered_map<string, bool> hasActedThisRound;
    int lastRaiser;
    std::unique_ptr<Poker> poker;
};

class Server {
private:
    int server_fd;
    int ws_server_fd;
    int nextGameID;
    std::vector<int> freeGameID;
    std::unordered_map<string, int> registeredPlayers;
    std::unordered_map<int, GameRoom> gameRooms;
    std::unordered_map<string, int> playerToGameID;
    std::unordered_map<int, ClientType> clientTypes;

    // WebSocket helper functions
    std::string base64Encode(const unsigned char* data, size_t len);
    bool performWebSocketHandshake(int client_fd);
    json receiveWebSocketMessage(int client_fd);
    void sendWebSocketMessage(int client_fd, const json& message);

    // Message type parsing
    MessageType getMessageType(const std::string &typeStr);
    ActionType getActionType(const std::string &typeStr);

    // Message handling
    json receiveMessage(int clientSocket);
    void sendMessage(int clientSocket, const json &message);
    void handleClient(int client_fd, ClientType clientType);

    // Command handlers
    void handleRegister(const json &request, int client_fd, std::string &client_username);
    void handleListGames(const json &request, int client_fd);
    void handleCreateGame(const json &request, int client_fd);
    void handleJoinGame(const json &request, int client_fd);
    void handleExitGame(const json &request, int client_fd);
    void handleUnregister(const json &request, int client_fd, std::string &client_username);
    void handleStartGame(const json& request, int client_fd);
    bool handleAction(const json& request, int client_fd);

    // Game logic helpers
    bool isBettingRoundComplete(GameRoom& room);
    void advanceGameStage(GameRoom& room);
    void handleShowdown(GameRoom& room);
    void broadcastGameState(GameRoom& room);

public:
    Server();
    ~Server();
    void run();
};

#endif // SERVER_H