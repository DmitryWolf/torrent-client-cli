#pragma once

#include "peer.h"
#include "torrent_file.h"
#include <string>
#include <vector>
#include <openssl/sha.h>
#include <fstream>
#include <variant>
#include <list>
#include <map>
#include <sstream>
#include <cpr/cpr.h>
#include <iostream>
#include <math.h>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <stack>
#include <assert.h>
#include <iostream>
#include <curl/curl.h>

#define debug(x, y) std::cerr << x << ":\t" << y << "\n";


namespace Bencode {

    enum flagForTorrentFile : int8_t {
        NOTHING = -1,
        OTHER,

        ANNOUNCE,
        ANNOUNCE_LIST,
        COMMENT,
        CREATED_BY,
        CREATION_DATE,
        INFO,
        FILES,
        LENGTH,
        MD5SUM,
        PATH,
        NAME,
        PIECE_LENGTH,
        PIECES,
        URL_LIST
    };
/*
 * В это пространство имен рекомендуется вынести функции для работы с данными в формате bencode.
 * Этот формат используется в .torrent файлах и в протоколе общения с трекером
*/
    Peer parsePeer(std::string data);
    std::string getCompactIp(const std::string& ip, bool isReversed = false);

    size_t getNumWithIncrementIdx(const std::string& data, size_t& idx);
    size_t getNumWithDecrementIdx(const std::string& data, size_t& idx);
    std::string getWord(const std::string& data, size_t& idx);
    std::pair<std::string, std::string> getPairOfLenAndWord(const std::string& data, size_t& idx);
    std::unordered_map<std::string, flagForTorrentFile> makeFlag();
    std::string getFileData(const std::string& filename);

    std::string parseIp(std::vector<int> v);
    size_t str2num(const std::string& s);
    size_t str2numRev(const std::string& s);
}
