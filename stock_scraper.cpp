#include <iostream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <vector>
#include <ctime>
#include <fstream>
#include <sstream>

using json = nlohmann::json;
using namespace std;

const string API_KEY = "C82n3zDKGmaxeluAKUrfggMfK_gOlbM2"; // Replace with your API key

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

// Convert Unix timestamp (milliseconds) to YYYY-MM-DD format
string convertTimestamp(long long timestamp) {
    time_t rawTime = timestamp / 1000; // Convert from milliseconds to seconds
    struct tm* timeInfo = localtime(&rawTime);
    
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d", timeInfo);
    return string(buffer);
}

// Function to fetch stock prices and store them in a map
unordered_map<string, pair<double, string>> fetchAllStockPrices(const vector<string>& tickers) {
    unordered_map<string, pair<double, string>> stockData;
    
    for (const string& ticker : tickers) {
        string url = "https://api.polygon.io/v2/aggs/ticker/" + ticker + "/prev?adjusted=true&apiKey=" + API_KEY;
        string jsonData = fetchStockData(url);
        
        try {
            auto data = json::parse(jsonData);
            if (data.contains("results") && !data["results"].empty()) {
                auto result = data["results"][0];
                double closePrice = result["c"];
                string date = convertTimestamp(result["t"]);
                
                stockData[ticker] = {closePrice, date};
                cout << "Fetched: " << ticker << " | $" << closePrice << " | " << date << endl;
            } else {
                cout << "Error fetching: " << ticker << endl;
            }
        } catch (...) {
            cout << "Invalid response for: " << ticker << endl;
        }
    }

    return stockData;
}

// Load S&P 500 tickers from a file
vector<string> loadSP500Tickers(const string& filename) {
    vector<string> tickers;
    ifstream file(filename);
    string line;

    while (getline(file, line)) {
        stringstream ss(line);
        string ticker;
        getline(ss, ticker, ','); // Assuming CSV format
        tickers.push_back(ticker);
    }

    return tickers;
}

int main() {
    cout << "Loading S&P 500 tickers..." << endl;
    vector<string> tickers = loadSP500Tickers("sp500_tickers.csv"); // Make sure this file exists

    cout << "Fetching stock prices..." << endl;
    unordered_map<string, pair<double, string>> stockPrices = fetchAllStockPrices(tickers);

    string input;
    while (true) {
        cout << "\nEnter a stock symbol (or type 'exit' to quit): ";
        cin >> input;

        if (input == "exit") break;

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
