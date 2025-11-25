#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

using namespace std;
enum class  MESSAGETYPE{
    LOGIN,
    REGISTER,
    CREATE_GAME,
    JOIN_GAME,
    EXIT_GAME,
    UNREGISTER,
    START_GAME,
    PLAY_TURN
};

struct PlayerInfo{
    char name[50];
    char password[50];
    int gameID; // -1 if not in a game
    int socket_fd;
};

struct GameRoom {
    int game_id;
    std::vector<int> player_fds;
    int max_players;
};
class client {
  public:
    void Login(char* name, char* password);
    void Register(char* name, char* password);
    void createGame();
    void joinGame(int gameID);
    void exitGame(int gameID);
    void unRegister(char* name, char* password);
    void StartGame(int gameID);
    void PlayTurn(int gameID, int turnData);
  private: 
    int server_fd;
    std::string username;
    bool registered;

    void sendDataToServer(const char* data);
};

void client::sendDataToServer(const char* data) {
    
}
void client::Login(char* name, char* password) {
    // Implementation for login
}




























int main()
{

    // creating socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    // specifying address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // sending connection request
    connect(clientSocket, (struct sockaddr*)&serverAddress,
            sizeof(serverAddress));

    char* name = new char[50];
    char* password = new char[50];

    // sending data
    const char* message = "Hello, server!";
    send(clientSocket, message, strlen(message), 0);

    // closing socket
    close(clientSocket);

    return 0;
}