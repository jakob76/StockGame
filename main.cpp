#include <iostream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <ctime>
#include <map>
#include <iomanip>
#include <limits>

using json = nlohmann::json;

// Callback function for cURL to store response
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    output->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Function to fetch data from Polygon.io
std::string fetchStockData(const std::string& url) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    return readBuffer;
}

// Convert Unix timestamp (milliseconds) to YYYY-MM-DD format
std::string convertTimestamp(long long timestamp) {
    time_t rawTime = timestamp / 1000; // Convert from milliseconds to seconds
    struct tm* timeInfo = localtime(&rawTime);
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d", timeInfo);
    return std::string(buffer);
}

// Structure to hold stock information
struct StockInfo {
    double price;
    std::string date;
};

// Function to fetch the previous close price for a stock
StockInfo getStockPrice(const std::string& ticker, const std::string& apiKey) {
    StockInfo info = {0.0, ""};
    std::string url = "https://api.polygon.io/v2/aggs/ticker/" + ticker + "/prev?adjusted=true&apiKey=" + apiKey;
    std::string jsonData = fetchStockData(url);
    
    try {
        // Parse JSON response
        auto data = json::parse(jsonData);
        if (data.contains("results") && !data["results"].empty()) {
            auto result = data["results"][0]; // Extract first (and only) result
            info.price = result["c"];
            info.date = convertTimestamp(result["t"]);
            std::cout << "Ticker: " << ticker << " | Previous Close Price: $" << info.price 
                      << " | Date: " << info.date << std::endl;
        } else {
            std::cout << "Error fetching stock price for " << ticker << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Error parsing data for " << ticker << ": " << e.what() << std::endl;
    }
    
    return info;
}

// Clear the input buffer
void clearInputBuffer() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

// Display portfolio
void displayPortfolio(const std::map<std::string, std::pair<int, double>>& portfolio, double cash) {
    double totalValue = cash;
    
    std::cout << "\n===== YOUR PORTFOLIO =====\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Cash: $" << cash << std::endl;
    
    if (portfolio.empty()) {
        std::cout << "You don't own any stocks yet.\n";
    } else {
        std::cout << "Stocks:\n";
        for (const auto& position : portfolio) {
            double positionValue = position.second.first * position.second.second;
            totalValue += positionValue;
            
            std::cout << position.first << ": " 
                      << position.second.first << " shares @ $" 
                      << position.second.second << " = $" 
                      << positionValue << std::endl;
        }
    }
    
    std::cout << "Total Portfolio Value: $" << totalValue << "\n";
    std::cout << "==========================\n\n";
}

int main() {
    std::string apiKey = "C82n3zDKGmaxeluAKUrfggMfK_gOlbM2"; // Replace with your valid API key
    double cash = 5000.0;
    std::map<std::string, std::pair<int, double>> portfolio; // ticker -> (shares, avg_price)
    std::string ticker;
    int choice = 0;
    
    std::cout << "===== STOCK MARKET INVESTMENT GAME =====\n";
    std::cout << "You have $5000 to invest in the stock market.\n";
    
    while (choice != 5) {
        std::cout << "\nWhat would you like to do?\n";
        std::cout << "1. Look up a stock price\n";
        std::cout << "2. Buy stocks\n";
        std::cout << "3. Sell stocks\n";
        std::cout << "4. View portfolio\n";
        std::cout << "5. Exit game\n";
        std::cout << "Enter your choice (1-5): ";
        
        std::cin >> choice;
        
        switch (choice) {
            case 1: {
                std::cout << "Enter stock ticker symbol (e.g., AAPL): ";
                std::cin >> ticker;
                
                // Convert ticker to uppercase
                for (char& c : ticker) {
                    c = toupper(c);
                }
                
                StockInfo info = getStockPrice(ticker, apiKey);
                break;
            }
            
            case 2: {
                std::cout << "Enter stock ticker symbol to buy: ";
                std::cin >> ticker;
                
                // Convert ticker to uppercase
                for (char& c : ticker) {
                    c = toupper(c);
                }
                
                StockInfo info = getStockPrice(ticker, apiKey);
                
                if (info.price > 0) {
                    int shares = 0;
                    double maxShares = std::floor(cash / info.price);
                    
                    std::cout << "You can afford up to " << maxShares << " shares.\n";
                    std::cout << "How many shares would you like to buy? ";
                    std::cin >> shares;
                    
                    if (shares <= 0) {
                        std::cout << "Invalid number of shares.\n";
                    } else if (shares * info.price > cash) {
                        std::cout << "Not enough cash to buy " << shares << " shares.\n";
                    } else {
                        double cost = shares * info.price;
                        cash -= cost;
                        
                        // Update portfolio
                        if (portfolio.find(ticker) != portfolio.end()) {
                            // Already own some of this stock, calculate new average price
                            int oldShares = portfolio[ticker].first;
                            double oldAvgPrice = portfolio[ticker].second;
                            double oldValue = oldShares * oldAvgPrice;
                            double newValue = oldValue + cost;
                            int newShares = oldShares + shares;
                            double newAvgPrice = newValue / newShares;
                            
                            portfolio[ticker] = std::make_pair(newShares, newAvgPrice);
                        } else {
                            // New position
                            portfolio[ticker] = std::make_pair(shares, info.price);
                        }
                        
                        std::cout << "Successfully bought " << shares << " shares of " << ticker 
                                  << " at $" << info.price << " per share.\n";
                        std::cout << "Total cost: $" << cost << "\n";
                        std::cout << "Remaining cash: $" << cash << "\n";
                    }
                }
                break;
            }
            
            case 3: {
                if (portfolio.empty()) {
                    std::cout << "You don't own any stocks to sell.\n";
                    break;
                }
                
                std::cout << "Enter stock ticker symbol to sell: ";
                std::cin >> ticker;
                
                // Convert ticker to uppercase
                for (char& c : ticker) {
                    c = toupper(c);
                }
                
                if (portfolio.find(ticker) == portfolio.end()) {
                    std::cout << "You don't own any shares of " << ticker << ".\n";
                    break;
                }
                
                StockInfo info = getStockPrice(ticker, apiKey);
                
                if (info.price > 0) {
                    int ownedShares = portfolio[ticker].first;
                    std::cout << "You own " << ownedShares << " shares of " << ticker << ".\n";
                    
                    int sharesToSell = 0;
                    std::cout << "How many shares would you like to sell? ";
                    std::cin >> sharesToSell;
                    
                    if (sharesToSell <= 0) {
                        std::cout << "Invalid number of shares.\n";
                    } else if (sharesToSell > ownedShares) {
                        std::cout << "You don't own that many shares.\n";
                    } else {
                        double revenue = sharesToSell * info.price;
                        cash += revenue;
                        
                        // Update portfolio
                        if (sharesToSell == ownedShares) {
                            // Sold all shares
                            portfolio.erase(ticker);
                        } else {
                            // Partial sale, average price stays the same
                            portfolio[ticker].first -= sharesToSell;
                        }
                        
                        std::cout << "Successfully sold " << sharesToSell << " shares of " << ticker 
                                  << " at $" << info.price << " per share.\n";
                        std::cout << "Total revenue: $" << revenue << "\n";
                        std::cout << "Cash balance: $" << cash << "\n";
                    }
                }
                break;
            }
            
            case 4:
                displayPortfolio(portfolio, cash);
                break;
                
            case 5:
                std::cout << "Thank you for playing the Stock Market Investment Game!\n";
                displayPortfolio(portfolio, cash);
                break;
                
            default:
                std::cout << "Invalid choice. Please enter a number between 1 and 5.\n";
                clearInputBuffer();
                break;
        }
    }
    
    return 0;
}