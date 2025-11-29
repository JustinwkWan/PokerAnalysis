#include "client.h"
#include <iostream>
using namespace std;

void testRegisterPlayers(client& myClient) {
    cout << "\n--- Test 1: Register ---" << endl;
    myClient.Register((char*)"PlayerOne");
    myClient.Register((char*)"PlayerTwo");
    myClient.Register((char*)"PlayerTwo");
}

void testCreateGames(client& myClient) {
  cout << "\n--- Test 2: Create Game" << endl; 
  myClient.createGame(10,20,1);
  myClient.createGame(10,15,2); // 
  myClient.createGame(5,2,3); // smaller big than small blind 
  myClient.createGame(10,10,1); // duplicate GameID
  myClient.createGame(10,-1,5);
}

void testListGame(client& myClient) {
  cout << "\n--- Test 3: List Games" << endl; 
  myClient.ListGames();
}
int main() {
  cout << "=== Poker Client Starting ===" << endl;
  client myClient; 
  testRegisterPlayers(myClient);
  testCreateGames(myClient);
  testListGame(myClient);
  return 0;
}