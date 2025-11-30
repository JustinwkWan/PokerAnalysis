#include "client.h"
#include <iostream>
using namespace std;

void testRegisterPlayers(client& myClient) {
    cout << "\nTest: Register" << endl;
    myClient.Register((char*)"PlayerOne");
    myClient.Register((char*)"PlayerTwo");
    myClient.Register((char*)"PlayerTwo");
}

void testCreateGames(client& myClient) {
  cout << "\nTest: Create Game" << endl; 
  myClient.createGame(10,20);
  myClient.createGame(10,15); // 
  myClient.createGame(5,2); // smaller big than small blind 
  myClient.createGame(10,10); // duplicate GameID
  myClient.createGame(10,-1);
}

void testListGame(client& myClient) {
  cout << "\nTest: List Games" << endl; 
  myClient.ListGames();
}

void testJoinGame(client& myClient) {
  cout << "\nTest: Join Games" << endl;
  myClient.joinGame((char*)"PlayerOne",1);
  myClient.joinGame((char*)"PlayerTwo",1);
  myClient.ListGames();
}

void testExitGame(client& myClient) {
  cout << "\nTest: Exit Game" << endl; 
  myClient.exitGame((char*)"PlayerOne");
  myClient.exitGame((char*)"PlayerOne");
  myClient.exitGame((char*)"FakePlayer");
  myClient.exitGame((char*)"PlayerTwo");
}

void testUnregister(client & myClient) {
  cout << "\nTest: Unregister" << endl; 
  myClient.unRegister((char*)"PlayerOne");
  myClient.unRegister((char*)"PlayerOne");
  myClient.unRegister((char*)"FakePlayer");
  myClient.unRegister((char*)"PlayerTwo");
}

int main() {
  cout << "=== Poker Client Starting ===" << endl;
  client myClient; 
  testRegisterPlayers(myClient);
  testCreateGames(myClient);
  testListGame(myClient);
  testJoinGame(myClient);
  testExitGame(myClient);
  testUnregister(myClient);
  return 0;
}