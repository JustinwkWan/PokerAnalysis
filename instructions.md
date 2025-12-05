# Install dependencies (if needed)
sudo apt-get update
sudo apt-get install -y build-essential libssl-dev nlohmann-json3-dev

# Build the server
make

# Start the poker server (in one terminal)
./poker_server

# Serve the frontend (in another terminal)
python3 -m http.server 8000

# Open your browser to:
# http://localhost:8000/index.html