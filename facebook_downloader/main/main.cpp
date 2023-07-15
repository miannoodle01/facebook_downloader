#include "../includes/includes.hpp"

int main(int argc, char* argv[]) {
    facebookDownloader fd;
    fd.getArgs(argc, argv);
    fd.checkUrl();
    fd.checkQuality();
    fd.downloadVideo();
    fd.helpCommand();
    return 0;
}