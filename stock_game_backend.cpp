#include <iostream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <ctime>
#include <map>
#include <fstream>
using json = nlohmann::json;

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    output->append((char*)contents, size * nmemb);
    return size * nmemb;
}

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

std::string convertTimestamp(long long timestamp) {
    time_t rawTime = timestamp / 1000;
    struct tm* timeInfo = localtime(&rawTime);
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d", timeInfo);
    return std::string(buffer);
}

double getStockPrice(const std::string& ticker, const std::string& apiKey) {
    std::string url = "https://api.polygon.io/v2/aggs/ticker/" + ticker + "/prev?adjusted=true&apiKey=" + apiKey;
    std::string data = fetchStockData(url);
    try {
        auto jsonData = json::parse(data);
        if (jsonData.contains("results") && !jsonData["results"].empty()) {
            return jsonData["results"][0]["c"];
        }
    } catch (...) {
        std::cerr << "Error fetching price for ticker: " << ticker << std::endl;
    }
    return 0.0;
}

int main(int argc, char* argv[]) {
    std::string apiKey = "C82n3zDKGmaxeluAKUrfggMfK_gOlbM2";
    if (argc < 2) {
        std::cerr << R"({"error": "Usage: stock_backend <action> <args>"})" << std::endl;
        return 1;
    }
    
    std::string action = argv[1];

    // Price lookup
    if (action == "price") {
        if (argc < 3) {
            std::cerr << R"({"error": "Please provide a ticker."})" << std::endl;
            return 1;
        }
        std::string ticker = argv[2];
        double price = getStockPrice(ticker, apiKey);
        if (price > 0) {
            json response = {{"ticker", ticker}, {"price", price}};
            std::cout << response.dump() << std::endl;
            return 0;
        } else {
            std::cout << R"({"error": "Failed to fetch price."})" << std::endl;
            return 1;
        }
    }

    // Portfolio value calculation
    else if (action == "portfolio") {
        if (argc < 3) {
            std::cerr << R"({"error": "Please provide a portfolio."})" << std::endl;
            return 1;
        }
        // Parse portfolio (it's passed as JSON in the argument)
        std::string portfolioJson = argv[2];
        json portfolio = json::parse(portfolioJson);
        
        double totalValue = 0.0;

        // Iterate through the portfolio and calculate total value
        for (auto& item : portfolio.items()) {
            std::string ticker = item.key();
            int shares = item.value()[0];  // Assumes format { ticker: [shares, avg_price] }
            double stockPrice = getStockPrice(ticker, apiKey);
            if (stockPrice > 0) {
                totalValue += stockPrice * shares;
            }
        }

        // Return total value of the portfolio
        json response = {{"total_value", totalValue}};
        std::cout << response.dump() << std::endl;
        return 0;
    }

    std::cerr << R"({"error": "Unknown action."})" << std::endl;
    return 1;
}
