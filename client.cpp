#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;
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
};

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