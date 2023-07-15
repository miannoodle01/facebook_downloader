#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <regex>
#include <curl/curl.h>
#include <boost/progress.hpp>

class facebookDownloader {
public:
    int getArgs(int argc, char *argv[]);
    int checkUrl();
    int checkQuality();
    int downloadVideo();
    void helpCommand();
    size_t curlCallback(
        char *contents,
        size_t size,
        size_t nmemb,
        std::string *response
    );
    size_t curlWriteCallBack(
        void* contents,
        size_t size,
        size_t nmemb,
        std::ostream* stream
    );
    size_t progressCallBack(
        void* clientp,
        double dltotal,
        double dlnow,
        double ultotal,
        double ulnow
    );
    std::string curlGetRequest(std::string &url);

private:
    char noValidArgMessage[30] = "No valid argument specified.\n";
    std::string videoQuality;
    std::string content;
    std::string url1;
    bool isSdAvailable = 0;
    bool isHdAvailable = 0;
    boost::progress_display* progress;
    char helpDesc[255] = "This tool coded to download facebook videos.\n" \
    "coded by mian [ noodle ].\n Usage: facebookDownloader -u example_url" \
    " -q example_quality\nOptions:\n-u      --url receives the videos link\n" \
    "-q     --quality receives the desired quality, the options are: \"hd\"" \
    " \"sd\"\n";
};

int facebookDownloader::getArgs(int argc, char *argv[]) {
    std::vector<std::string> args(argv + 1, argv + argc);
    for(size_t i = 0; i < args.size(); i++) {
        if(args[i] == "--url" || args[i] == "-u"){
            url1 = args[i + 1];
            //i++;
        } else if(args[i] == "--quality" || args[i] == "-q") {
            videoQuality = args[i + 1];
            //i++;
        } else if (args[i] == "--help" || args[i] == "-h") {
            helpCommand();
            exit(EXIT_SUCCESS);
        } else {
            fprintf(stderr, noValidArgMessage);
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}
 
int facebookDownloader::checkUrl() {
    std::regex pattern("^(https:|)[/][/]www.([^/]+[.])*facebook.com");

    bool isMatched = std::regex_match(url1, pattern);
    if(isMatched == 1) {
        printf("entered url is valid.\n");
    } else {
        fprintf(stderr, "entered url is not valid, exiting...\n");
        exit(EXIT_FAILURE);
    }
    return 0;
}

int facebookDownloader::checkQuality() {
    content = curlGetRequest(url1);
    std::regex sdPattern("sd_src:\"https");
    std::regex hdPattern("hd_src:\"https");
    std::smatch match;
    if(std::regex_search(content, match, sdPattern)) {
        isSdAvailable = 1;
    }
    
    if(std::regex_search(content, match, hdPattern)) {
        isHdAvailable = 1;
    }

    if(
        (videoQuality == "hd" && isHdAvailable == 0) ||
        (videoQuality == "sd" && isSdAvailable == 0)
    ) {
        fprintf(stderr, "desired quality is not available, exiting...");
    }
    return 0;
}

int facebookDownloader::downloadVideo() {
    char dateString[12];
    std::time_t currentTime = std::time(nullptr);
    std::strftime(
        dateString,
        sizeof(dateString),
        "%y%m%d%H%M%S",
        std::localtime(&currentTime)
    );
    std::string fileName = dateString;
    fileName.append(".mp4");

    std::regex urlRegex;
    if(std::strcmp("sd", videoQuality.c_str()) == 0) {
        urlRegex = std::regex("sd_src:\"(.+?)\"");
    } else if(std::strcmp("hd", videoQuality.c_str()) == 0) {
        urlRegex = std::regex("hd_src:\"(.+?)\"");
    } else {
        fprintf(stderr, "entered quality is inappropriate, exiting...");
        exit(EXIT_FAILURE);
    }

    std::smatch urlMatch;
    std::regex_search(content, urlMatch, urlRegex);
    std::string finalUrl = urlMatch[1].str();

    CURL *curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, finalUrl);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, &facebookDownloader::progressCallBack);
        curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &progress);

        std::fstream file(
            fileName,
            std::ios::out | std::ios::trunc | std::ios::binary
        );
        if (file.is_open()) {
            curl_easy_setopt(
                curl,
                CURLOPT_WRITEFUNCTION,
                &facebookDownloader::curlCallback
            );

            curl_easy_setopt(
                curl,
                CURLOPT_WRITEDATA,
                &file
            );

            CURLcode res = curl_easy_perform(curl);
            if(res != CURLE_OK) {
                fprintf(
                    stderr,
                    "error occurred on writing data to file, exiting...\n"
                );
            }
            file.close();
            curl_easy_cleanup(curl);
            printf("file downloaded successfully.\n");
        }
    }
    return 0;
}

void facebookDownloader::helpCommand() {
    printf(helpDesc);
}

size_t facebookDownloader::curlCallback(
    char *contents,
    size_t size,
    size_t nmemb,
    std::string *response
) {
    size_t totalSize = size* nmemb;
    response->append(contents, totalSize);
    return totalSize;
}

size_t facebookDownloader::curlWriteCallBack(
    void* contents,
    size_t size,
    size_t nmemb,
    std::ostream* stream
) {
    stream->write(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

size_t facebookDownloader::progressCallBack(
    void* clientp,
    double dltotal,
    double dlnow,
    double ultotal,
    double ulnow
) {
    progress = static_cast<boost::progress_display*>(clientp);
    if (dltotal > 0) {
        progress->restart(static_cast<unsigned long>(dltotal));
    }
    ++(*progress);
    return 0;
}

std::string facebookDownloader::curlGetRequest(std::string &url) {
    CURL* curl = curl_easy_init();
    std::string response;

    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &facebookDownloader::curlCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        CURLcode res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            fprintf(
                stderr,
                "CURL request failed: %s, exiting...\n",
                curl_easy_strerror(res)
            );
            exit(EXIT_FAILURE);
        }
        curl_easy_cleanup(curl);
    }

    return response;
}