#include "client.h"
#include <iostream>
using namespace std;
int main() {
  cout << "=== Poker Client Starting ===" << endl;
  
  try {
    // Test 1: Create client and register
    cout << "\n--- Test 1: Register PlayerOne ---" << endl;
    client myClient;
    myClient.Register((char*)"PlayerOne");
    
    // Give server time to process
    sleep(1);
    
    cout << "\n--- Test Complete ---" << endl;
    
  } catch (const std::exception& e) {
    cerr << "Error: " << e.what() << endl;
    return 1;
  }
  
  return 0;
}