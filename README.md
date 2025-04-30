# Stock Game
Project By Damon Cawthon and Jakob Nguyen
# Background
Stock Game will be a stock market-based real-time game based primarily in C++ and hosted through php. It attempts to give users a low-stakes practice to help understand the market. Users will benefit from trying out new investment strategies in a safe environment.
# Investing
If choosing a new game, the user will begin with a budget of $5000. Using daily S&P 500 updates, the user will have options to invest this budget into hundreds of stocks in real time. Upon investment, a file will be written with the user's portfolio, budget, and information about the stocks they invested in.
# Withdrawing
When loading in an old save file, the user's portfolio and budget are updated. With access to new market information, the user can choose which stocks of theirs to sell (and in what quantities). Upon selling, the user will be given a detailed report about the stock, including their net gain, return percent, and adjustments for inflation.


# Packages and compilation
// Packages

sudo apt-get update
sudo apt-get install nlohmann-json3-dev

// Compiler

g++ -std=c++17 main.cpp -o main -Iinclude -lcurl
./main


// Stock game backend compiler
g++ -o stock_game_backend stock_game_backend.cpp -lcurl -I/opt/homebrew/opt/nlohmann-json/include

// php localhost startup
php -S localhost:8000
