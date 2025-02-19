#include <iostream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <string>
#include <algorithm>
#include <cctype>

using json = nlohmann::json;
using namespace std;

// Callback function for cURL to store response
size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* output) {
    output->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Function to fetch data from Polygon.io
string fetchStockData(const string& url) {
    CURL* curl;
    CURLcode res;
    string readBuffer;

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

// Function to fetch the latest trade price for a stock
void getStockPrice(const string& ticker, const string& apiKey, unordered_map<string, pair<double, string>>& stockPrices) {
    string url = "https://api.polygon.io/v2/last/trade/" + ticker + "?apiKey=" + apiKey;
    string jsonData = fetchStockData(url);

    // Parse JSON response
    auto data = json::parse(jsonData);
    if (data.contains("results")) {
        stockPrices[ticker] = make_pair(data["results"]["p"], to_string(data["results"]["t"]));
    }
}

// Function to load S&P 500 tickers from a CSV file
vector<string> loadSP500Tickers(const string& fileName) {
    vector<string> tickers;
    ifstream file(fileName);
    string line;
    while (getline(file, line)) {
        transform(line.begin(), line.end(), line.begin(), ::toupper);  // Ensure uppercase
        tickers.push_back(line);
    }
    return tickers;
}

// Function to fetch prices for all S&P 500 tickers
unordered_map<string, pair<double, string>> fetchAllStockPrices(const vector<string>& tickers, const string& apiKey) {
    unordered_map<string, pair<double, string>> stockPrices;
    for (const auto& ticker : tickers) {
        getStockPrice(ticker, apiKey, stockPrices);
    }

    // Debug: Print all stored stock prices to check if they were loaded correctly
    cout << "\n--- Stock Prices Stored ---" << endl;
    for (const auto& pair : stockPrices) {
        cout << pair.first << " | " << pair.second.first << " | " << pair.second.second << endl;
    }

    return stockPrices;
}

// Function to convert input to uppercase
string toUpper(const string& str) {
    string result = str;
    transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

int main() {
    string apiKey = "YOUR_API_KEY_HERE";  // Replace with your Polygon.io API Key

    cout << "Loading S&P 500 tickers..." << endl;
    vector<string> tickers = loadSP500Tickers("sp500_tickers.csv");  // Ensure this file exists

    cout << "Fetching stock prices..." << endl;
    unordered_map<string, pair<double, string>> stockPrices = fetchAllStockPrices(tickers, apiKey);

    string input;
    while (true) {
        cout << "\nEnter a stock symbol (or type 'exit' to quit): ";
        cin >> input;
        input = toUpper(input);  // Ensure user input matches stored tickers

        if (input == "EXIT") break;

        if (stockPrices.find(input) != stockPrices.end()) {
            cout << "Ticker: " << input 
                 << " | Previous Close Price: $" << stockPrices[input].first 
                 << " | Date: " << stockPrices[input].second << endl;
        } else {
            cout << "Stock symbol not found. Try again!" << endl;
        }
    }

    cout << "Goodbye!" << endl;
    return 0;
}
