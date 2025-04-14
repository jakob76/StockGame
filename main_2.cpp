#include <iostream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <ctime>
#include <iomanip>
#include <string>

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

struct StockInfo {
    double price;
    std::string date;
};

StockInfo getStockPrice(const std::string& ticker, const std::string& apiKey) {
    StockInfo info = {0.0, ""};
    std::string url = "https://api.polygon.io/v2/aggs/ticker/" + ticker + "/prev?adjusted=true&apiKey=" + apiKey;
    std::string jsonData = fetchStockData(url);

    try {
        auto data = json::parse(jsonData);
        if (data.contains("results") && !data["results"].empty()) {
            auto result = data["results"][0];
            info.price = result["c"];
            info.date = convertTimestamp(result["t"]);
        }
    } catch (...) {
        // Fail silently for now
    }

    return info;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: ./stock_game <action> <ticker> [shares]" << std::endl;
        return 1;
    }

    std::string action = argv[1];
    std::string ticker = argv[2];
    int shares = argc >= 4 ? std::stoi(argv[3]) : 0;
    std::string apiKey = "YOUR_POLYGON_API_KEY";  // Replace with your key

    if (action == "price") {
        StockInfo info = getStockPrice(ticker, apiKey);
        if (info.price > 0) {
            std::cout << "Ticker: " << ticker << " | Price: $" << std::fixed << std::setprecision(2) << info.price
                      << " | Date: " << info.date << std::endl;
        } else {
            std::cout << "Error fetching data for " << ticker << std::endl;
        }
    } else if (action == "buy") {
        std::cout << "Simulated buy: " << shares << " shares of " << ticker << std::endl;
    } else if (action == "sell") {
        std::cout << "Simulated sell: " << shares << " shares of " << ticker << std::endl;
    } else {
        std::cout << "Invalid action. Use price, buy, or sell." << std::endl;
    }

    return 0;
}
