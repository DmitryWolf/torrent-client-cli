#include "torrent_file.h"
#include "bencode.h"
#include <vector>
#include <openssl/sha.h>
#include <fstream>
#include <variant>
#include <sstream>
#include <unordered_map>
#include "byte_tools.h"
using namespace Bencode;


TorrentFile parser(std::string& data){
    std::stack<char> op;
    TorrentFile tf;
    flagForTorrentFile flag = NOTHING;
    std::unordered_map<std::string, flagForTorrentFile> um = makeFlag();
    size_t num = 0;
    std::string infoString = "";
    int cnt = 0;
    int infoCounter = 0;
    int8_t isFiles = 0;
    std::string prevStr = "";
    File file;
    size_t ds = data.size();
    std::string pathRoad = "";
    for (size_t i = 0; i < data.size(); ++i){
        char c = data[i];
        size_t oldI = i;

        if (c == 'd') { // type == dict
            if (flag == flagForTorrentFile::INFO){
                infoCounter++;
                flag = flagForTorrentFile::NOTHING;
            } else if (flag != flagForTorrentFile::NOTHING || isFiles == 1){
                cnt++;
                if (infoCounter > 0){
                    infoCounter++;
                }
            }
            op.push(c);
        } else if (c == 'i') { // type == integer
            cnt++;
            if (infoCounter > 0){
                infoCounter++;
            }
            op.push('i');
        } else if (c == 'l') { // type == list
            if (flag != flagForTorrentFile::NOTHING || isFiles == 1){
                cnt++;
                if (infoCounter > 0){
                    infoCounter++;
                }
            }
            op.push('l');
        } else if (c == 'e') { // end of type
            if (file.path != "" && file.length != 0) {
                tf.files.push_back(file);
                file.clear();
            }
            else if (pathRoad != "") {
                file.path = pathRoad;
                pathRoad = "";
                flag = flagForTorrentFile::NOTHING;
            }
            if (cnt > 0) {
                cnt--;
            }
            if (cnt == 0){
                flag = flagForTorrentFile::NOTHING;
                if (isFiles == 1) {
                    isFiles = -1;
                }
            }
            if (infoCounter > 0){
                infoCounter--;
                if (infoCounter == 0) { // last symbol of info
                    infoString += c;
                }
            }
            op.pop();
        } else if (c >= '0' && c <= '9') { // type == string
            if (op.top() == 'i') { // нужно получить число
                num = getNumWithDecrementIdx(data, i);
                if (flag == flagForTorrentFile::CREATION_DATE) {
                    tf.creation_date = num;
                } else if (flag == flagForTorrentFile::LENGTH) {
                    if (isFiles != 0) {
                        file.length = num;
                    } else {
                        tf.length = num;
                    }
                } else if (flag == flagForTorrentFile::PIECE_LENGTH) {
                    tf.pieceLength = num;
                }
                flag = flagForTorrentFile::NOTHING;
                num = 0;
            }
            else{
                std::pair<std::string, std::string> s = getPairOfLenAndWord(data, i);
                
                // TODO: охарактеризировать строки
                if (flag == flagForTorrentFile::NOTHING) {
                    if (um.find(s.second) == um.end()){
                        flag = flagForTorrentFile::OTHER;
                        prevStr = s.second;
                    } else{
                        flag = um[s.second];
                        if (flag == flagForTorrentFile::FILES) {    
                            isFiles = 1;
                            flag = flagForTorrentFile::NOTHING;
                        }
                    }
                } else if (flag == flagForTorrentFile::ANNOUNCE || flag == flagForTorrentFile::ANNOUNCE_LIST) {
                    tf.announce_list.push_back(s.second);
                    if (flag == flagForTorrentFile::ANNOUNCE) {
                        flag = flagForTorrentFile::NOTHING;
                    }
                } else if (flag == flagForTorrentFile::COMMENT) {
                    tf.comment = s.second;
                    flag = flagForTorrentFile::NOTHING;
                } else if (flag == flagForTorrentFile::CREATED_BY) {
                    tf.created_by = s.second;
                    flag = flagForTorrentFile::NOTHING;
                } else if (flag == flagForTorrentFile::OTHER) {
                    tf.otherInfo.push_back(std::make_pair(prevStr, s.second));
                    prevStr = "";
                    flag = flagForTorrentFile::NOTHING;
                } else if (flag == flagForTorrentFile::PATH) {
                    if (pathRoad != "") {
                        pathRoad += "/";
                    }
                    pathRoad += s.second;
                } else if (flag == flagForTorrentFile::MD5SUM) {
                    if (isFiles != 0) {
                        file.md5sum = s.second;
                    } else {
                        tf.md5sum = s.second;
                    }
                    flag = flagForTorrentFile::NOTHING;
                } else if (flag == flagForTorrentFile::NAME) {
                    tf.name = s.second;
                    flag = flagForTorrentFile::NOTHING;
                } else if (flag == flagForTorrentFile::PIECES) {
                    size_t pieces_size = stoll(s.first);
                    for (size_t idx = 0; idx < pieces_size; idx += 20){
                        tf.pieceHashes.push_back(s.second.substr(idx, 20));
                    }
                    flag = flagForTorrentFile::NOTHING;
                } else if (flag == flagForTorrentFile::URL_LIST) {
                    tf.url_list.push_back(s.second);
                }
            }

        }

        if (infoCounter > 0){
            for (size_t j = oldI; j <= i; ++j){
                infoString += data[j];
            }
        }
    }
    tf.infoHash = CalculateSHA1(infoString);
    
    if (tf.files.empty()) {
        tf.files.push_back(File(tf.length, tf.name, tf.md5sum));
    }
    if (tf.length == 0){
        tf.length = tf.pieceHashes.size() * tf.pieceLength;
    }
    std::set<std::string> unique_strings(tf.announce_list.begin(), tf.announce_list.end());
    tf.announce_list.clear();
    for (const auto& it : unique_strings){
        tf.announce_list.push_back(it);
    }
    return tf;
}
TorrentFile LoadTorrentFile(const std::string& filename) {
    std::string data = getFileData(filename);
    TorrentFile result = parser(data);
    return result;
}