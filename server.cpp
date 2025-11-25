// C++ program to show the example of server application in
// socket programming
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include "json.hpp"
using json = nlohmann::json;

using namespace std;

class Server
{

public:
  Server();
  ~Server();
  void StartServer(int port);
  void AcceptClient();
  void ReceiveData(int clientSocket);
  void SendData(int clientSocket, const char *data);
  void StopServer();
  void handleClient();
private:
  int server_fd;
  struct sockaddr_in address;
  int opt = 1;
  int addrlen = sizeof(address);

  int client_fd;
  json recieveMessage(int clientSocket);
  void sendMessage(int clientSocket, const json& data);
  char buffer[1024] = {0};
};

Server::Server()
{
  // creating socket file descriptor
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == 0)
  {
    cerr << "Socket creation error" << endl;
    exit(EXIT_FAILURE);
  }
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // binding socket.
    bind(server_fd, (struct sockaddr*)&serverAddress,
         sizeof(serverAddress));

    // listening to the assigned socket
    listen(server_fd, 5);

    // accepting connection request
    int clientSocket
        = accept(server_fd, nullptr, nullptr);
  // attaching socket to the port
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                 &opt, sizeof(opt)))
  {
    cerr << "setsockopt error" << endl;
    exit(EXIT_FAILURE);
  }
}

Server::~Server()
{
  close(server_fd);
}

json Server::recieveMessage(int clientSocket)
{
  uint32_t msg_length_net;
  ssize_t received = recv(client_fd, &msg_length_net, sizeof(msg_length_net), MSG_WAITALL);
  if (received <= 0)
  {
    throw std::runtime_error("Client disconnected");
  }

  uint32_t msg_length = ntohl(msg_length_net);
  std::vector<char> buffer(msg_length);

  received = recv(client_fd, buffer.data(), msg_length, MSG_WAITALL);
  if (received <= 0)
  {
    throw std::runtime_error("Client disconnected");
  }

  std::string json_str(buffer.begin(), buffer.end());
  return json::parse(json_str);
}

void Server::sendMessage(int clientSocket, const json& message)
{
  std::string jsonString = message.dump();
  uint32_t length = htonl(jsonString.size());

  ssize_t bytesSent = send(clientSocket, &length, sizeof(length), 0);
  if (bytesSent != sizeof(length))
  {
    cerr << "Error sending message length" << endl;
    return;
  }

  bytesSent = send(clientSocket, jsonString.c_str(), jsonString.length(), 0);
  if (bytesSent != (ssize_t)jsonString.length())
  {
    throw std::runtime_error("Failed to send message");
  }
}

void Server::handleClient() {
  try {
    while (true) {
      json received = recieveMessage(client_fd);
      cout << "Received: " << received.dump() << endl;
      json response = {
        {"status", "SUCCESS"},
        {"echo", received}
      };
      // Echo back the received message
      sendMessage(client_fd, received);
    }
  } catch (const std::exception& e) {
    cerr << "Client handling error: " << e.what() << endl;
    close(client_fd);
  }
}

int main()
{
  // creating socket
  int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

  // specifying the address
  sockaddr_in serverAddress;
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(8080);
  serverAddress.sin_addr.s_addr = INADDR_ANY;

  // binding socket.
  bind(serverSocket, (struct sockaddr *)&serverAddress,
       sizeof(serverAddress));

  // listening to the assigned socket
  listen(serverSocket, 5);

  // accepting connection request
  int clientSocket = accept(serverSocket, nullptr, nullptr);

  // recieving data
  char buffer[1024] = {0};
  recv(clientSocket, buffer, sizeof(buffer), 0);
  cout << "Message from client: " << buffer
       << endl;

  // closing the socket.
  close(serverSocket);

  return 0;
}