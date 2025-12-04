Installs

On Windows:
curl -O https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp

On Linux: 
sudo yum install nlohmann-json3-dev
sudo yum install openssl-devel

To Run 
cd Server
make 
./poker_server

In a new terminal 
cd Server 
python3 -m http.server 8000