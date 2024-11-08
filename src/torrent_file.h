#pragma once

#include <string>
#include <vector>

struct File {
    File(size_t length, const std::string& path, const std::string& md5sum = "") :
        length(length), path(path), md5sum(md5sum) {}
        
    File() {
        clear();
    }
    void clear() {
        length = 0;
        md5sum = "";
        path = "";
    }
    size_t length;
    std::string md5sum;
    std::string path;
};


struct TorrentFile {
    std::vector<std::string> announce_list;
    std::string comment;
    std::string created_by;
    size_t creation_date = 0;
    std::string name;
    std::string md5sum;
    std::vector<std::string> pieceHashes;
    size_t pieceLength = 0;
    size_t length = 0;
    std::string infoHash;
    std::vector<std::pair<std::string, std::string>> otherInfo;
    std::vector<File> files;
    std::vector<std::string> url_list;
};


TorrentFile LoadTorrentFile(const std::string& filename);

TorrentFile parseData(const std::string& data);