#include <iostream>
#include "server.h"

using namespace std;

int main() {
  cout << "=== Poker Server Starting ===" << endl;
  
  Server server;
  server.handleClient();
  
  return 0;
}