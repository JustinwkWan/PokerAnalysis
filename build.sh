g++ client.cpp clientmain.cpp -o client 
g++ server.cpp servermain.cpp -o server
echo "Starting server"
./server 

sleep (1)

echo "Starting client"
./client