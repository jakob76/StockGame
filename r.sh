#!/bin/bash

# Update package list and install necessary packages if not already installed
echo "Updating package list..."
sudo apt-get update -y

# Install nlohmann-json3-dev if it's not already installed
if ! dpkg -l | grep -q nlohmann-json3-dev; then
    echo "Installing nlohmann-json3-dev..."
    sudo apt-get install -y nlohmann-json3-dev
else
    echo "nlohmann-json3-dev is already installed."
fi

# Install curl if it's not already installed
if ! dpkg -l | grep -q libcurl4-openssl-dev; then
    echo "Installing libcurl4-openssl-dev..."
    sudo apt-get install -y libcurl4-openssl-dev
else
    echo "libcurl4-openssl-dev is already installed."
fi

# Compile the C++ backend
echo "Compiling stock_game_backend.cpp..."
g++ -o stock_game_backend stock_game_backend.cpp -lcurl -I/opt/homebrew/opt/nlohmann-json/include

# Check if the compilation was successful
if [ $? -eq 0 ]; then
    echo "Compilation successful."
else
    echo "Compilation failed. Exiting."
    exit 1
fi

# Start the PHP built-in server
echo "Starting PHP server..."
php -S localhost:8000

# Open the browser (use appropriate method based on OS)
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    xdg-open http://localhost:8000
elif [[ "$OSTYPE" == "darwin"* ]]; then
    open http://localhost:8000
elif [[ "$OSTYPE" == "cygwin" || "$OSTYPE" == "msys" || "$OSTYPE" == "win32" ]]; then
    start http://localhost:8000
else
    echo "Unknown OS. Please open your browser and navigate to http://localhost:8000"
fi