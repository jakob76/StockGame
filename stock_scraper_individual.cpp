#include <iostream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <ctime>

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

// Function to fetch the previous close price for a stock
void getStockPrice(const std::string& ticker, const std::string& apiKey) {
    std::string url = "https://api.polygon.io/v2/aggs/ticker/" + ticker + "/prev?adjusted=true&apiKey=" + apiKey;
    std::string jsonData = fetchStockData(url);

    // Parse JSON response
    auto data = json::parse(jsonData);
    if (data.contains("results") && !data["results"].empty()) {
        auto result = data["results"][0]; // Extract first (and only) result
        
        std::cout << "Ticker: " << ticker 
                  << " | Previous Close Price: $" << result["c"] 
                  << " | Date: " << convertTimestamp(result["t"]) << std::endl;
    } else {
        std::cout << "Error fetching stock price for " << ticker << std::endl;
    }
}

int main() {
    std::string apiKey = "C82n3zDKGmaxeluAKUrfggMfK_gOlbM2";  // Replace with your valid API key

    // Fetch stock prices for AAPL, MSFT, GOOGL
    getStockPrice("AAPL", apiKey);
    getStockPrice("MSFT", apiKey);
    getStockPrice("GOOGL", apiKey);

    return 0;
}