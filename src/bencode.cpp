#include "bencode.h"

namespace Bencode {
    Peer parsePeer(std::string data){
        Peer ret;
        for (size_t i = 0; i <= 3; i++){
            ret.ip += std::to_string(reinterpret_cast<unsigned char&>(data[i]));
            if (i != 3) {
                ret.ip += ".";
            }
        }
        ret.port = (reinterpret_cast<unsigned char&>(data[4]) << 8) + reinterpret_cast<unsigned char&>(data[5]);
        return ret;
    }
    std::string getCompactIp(const std::string& ip, bool isReversed){
        std::string compactIp = "";
        size_t idx = 0;
        for (int i = 0; i < 4; i++){
            compactIp += char(getNumWithIncrementIdx(ip, idx));
        }
        if (isReversed){
            reverse(compactIp.begin(), compactIp.end());
        }
        return compactIp;
    }





    size_t getNumWithDecrementIdx(const std::string& data, size_t& idx){
        size_t num = 0;
        while (idx < data.size() && data[idx] >= '0' && data[idx] <= '9'){
            num *= 10;
            num += (data[idx] - '0');
            idx++;
        }
        idx--;
        return num;
    }

    size_t getNumWithIncrementIdx(const std::string& data, size_t& idx){
        size_t num = 0;
        while (idx < data.size() && data[idx] >= '0' && data[idx] <= '9'){
            num *= 10;
            num += (data[idx] - '0');
            idx++;
        }
        idx++;
        return num;
    }

    std::string getFileData(const std::string& filename){
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()){
            throw std::runtime_error("Failed to open file");
        }
        std::ostringstream buffer;
        buffer << file.rdbuf();
        std::string data = buffer.str();
        return data;
    }

    std::string getWord(const std::string& data, size_t& idx){
        std::string back = "";
        while (data[idx] >= 'a' && data[idx] <= 'z') {
            back += data[idx];
            idx++;
        }
        idx--;
        return back;
    }

    std::pair<std::string, std::string> getPairOfLenAndWord(const std::string& data, size_t& idx){
        std::pair<std::string, std::string> back = {"", ""};
        size_t len = 0;
        while (data[idx] >= '0' && data[idx] <= '9'){
            len *= 10;
            len += static_cast<size_t>(data[idx] - '0');
            idx++;
        }
        back.first = std::to_string(len);
        for (size_t i = 0; i < len; i++){
            back.second += data[idx + 1];
            idx++;
        }
        return back;
    }


    std::unordered_map<std::string, flagForTorrentFile> makeFlag(){
        std::unordered_map<std::string, flagForTorrentFile> back;
        back["announce"] = flagForTorrentFile::ANNOUNCE;
        back["announce-list"] = flagForTorrentFile::ANNOUNCE_LIST;
        back["comment"] = flagForTorrentFile::COMMENT;
        back["created by"] = flagForTorrentFile::CREATED_BY;
        back["creation date"] = flagForTorrentFile::CREATION_DATE;
        back["info"] = flagForTorrentFile::INFO;
        back["files"] = flagForTorrentFile::FILES;
        back["md5sum"] = flagForTorrentFile::MD5SUM;
        back["length"] = flagForTorrentFile::LENGTH;
        back["path"] = flagForTorrentFile::PATH;
        back["name"] = flagForTorrentFile::NAME;
        back["piece length"] = flagForTorrentFile::PIECE_LENGTH;
        back["pieces"] = flagForTorrentFile::PIECES;
        back["url-list"] = flagForTorrentFile::URL_LIST;
        
        return back;
    }






    std::string parseIp(std::vector<int> v) {
        std::string ans = "";
        for (int i = 0; i < (int)v.size(); i++) {
            ans += std::to_string(v[i]);
            if (i + 1 < (int)v.size()) {
                ans += ".";
            }
        }
        return ans;
    }

    size_t str2num(const std::string& s) {
        size_t k = 0;
        for (int i = 0; i < (int)(s.size()); i++) {
            k *= 10;
            k += (s[i] - '0');
        }
        return k;
    }
    size_t str2numRev(const std::string& s) {
        size_t k = 0;
        for (int i = (int)(s.size() - 1); i >= 0; i--) {
            k *= 10;
            k += (s[i] - '0');
        }
        return k;
    }
}
