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

int main(int argc, char* argv[]) {
    std::string apiKey = "C82n3zDKGmaxeluAKUrfggMfK_gOlbM2";
    if (argc < 3) {
        std::cerr << R"({"error": "Usage: stock_backend <action> <ticker> [shares] [savefile]"})" << std::endl;
        return 1;
    }
    std::string action = argv[1];
    std::string ticker = argv[2];
    int shares = argc > 3 ? std::stoi(argv[3]) : 0;
    std::string saveFile = argc > 4 ? argv[4] : "portfolio_save.json";
    
    if (action == "price") {
        std::string url = "https://api.polygon.io/v2/aggs/ticker/" + ticker + "/prev?adjusted=true&apiKey=" + apiKey;
        std::string data = fetchStockData(url);
        try {
            auto jsonData = json::parse(data);
            if (jsonData.contains("results") && !jsonData["results"].empty()) {
                auto result = jsonData["results"][0];
                double price = result["c"];
                std::string date = convertTimestamp(result["t"]);
                json response = {{"ticker", ticker}, {"price", price}, {"date", date}};
                std::cout << response.dump();
                return 0;
            }
        } catch (...) {
            std::cout << R"({"error": "Invalid response from API"})";
            return 1;
        }
    }
    else if (action == "load") {
        std::ifstream file(saveFile);
        if (file) {
            json data;
            file >> data;
            std::cout << data.dump();
        } else {
            std::cout << R"({"error": "Save file not found"})";
        }
    }
    else if (action == "save") {
        std::ofstream file(saveFile);
        if (file) {
            json data;
            std::cin >> data;
            file << data.dump(4);
            std::cout << R"({"status": "Save successful"})";
        } else {
            std::cout << R"({"error": "Could not open file to save"})";
        }
    }
    
    return 0;
}