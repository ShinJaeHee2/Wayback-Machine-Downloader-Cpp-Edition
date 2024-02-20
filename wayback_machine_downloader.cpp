#define _CRT_SECURE_NO_WARNINGS
#define TIMESTAMP 1
#define URL 2
#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

std::filesystem::path downloadPath;

size_t callback(void* ptr, size_t size, size_t nmemb, std::string* data) {
    data->append((char*)ptr, size * nmemb);
    return size * nmemb;
}

size_t writeFile(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

void get(std::string url) {
    CURL* curl = curl_easy_init();
    FILE* fp;
    std::vector<std::string> split;
    std::istringstream name(url);
    std::string splitName;

    while (getline(name, splitName, '/')) {
        split.push_back(splitName);
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFile);

    fp = fopen(split.back().c_str(), "wb");
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_perform(curl);
    std::cout << "downloaded " << split.back() << std::endl;
    fclose(fp);
    curl_easy_cleanup(curl);
 }



int main(int argc, char* argv[]) {

    if (argc < 2) {
        std::cout << "usage: wayback_machine_downloader [url link]" << std::endl;
        return 0;
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);

    CURL* curl = curl_easy_init();

    std::string cdxStart = "https://web.archive.org/cdx/search/cdx?url=";
    std::string cdxEnd = "&matchType=prefix&filter=statuscode:200&output=json";
    std::string urlStart = "https://web.archive.org/web/";
    std::string urlMiddle = "if_/";

    std::string fullUrl = cdxStart + argv[1] + cdxEnd;

    downloadPath = std::filesystem::current_path();

    if (!std::filesystem::exists("Folder")) {
        std::filesystem::create_directory("Folder");
    }

    std::filesystem::current_path("Folder");

    std::string response;

    nlohmann::json data;
    nlohmann::json index;

    std::string crawlUrl;

    if (!curl) {
        return 1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, fullUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 50L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    curl_easy_perform(curl);

    data = nlohmann::json::parse(response);

    for (nlohmann::json::iterator info = data.begin() + 1; info != data.end(); ++info) {
        index = *info;
        crawlUrl = urlStart + (std::string)index[TIMESTAMP] + urlMiddle + (std::string)index[URL];
        get(crawlUrl);
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();
    curl = NULL;

    return 0;
}