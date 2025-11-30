g++ client.cpp clientmain.cpp -o client 
g++ server.cpp servermain.cpp -o server
echo "Starting client and server"
./server & ./client && fg 