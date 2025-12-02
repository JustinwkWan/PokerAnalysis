#include "server.h"

int main() {
    try {
        Server server;
        server.run();  // This will run forever, accepting clients
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
