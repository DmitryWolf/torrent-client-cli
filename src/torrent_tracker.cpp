#include "torrent_tracker.h"
#include "bencode.h"
#include "byte_tools.h"
#include <cpr/cpr.h>

using namespace Bencode;

TorrentTracker::TorrentTracker(const std::vector<std::string>& urls){
    urls_ = urls;
}

void TorrentTracker::UpdatePeers(const TorrentFile& tf, std::string peerId, int port){
    for (const std::string& announce : tf.announce_list) {
        std::cout << "Connecting to tracker " << announce << std::endl;
        cpr::Response res = cpr::Get(
                cpr::Url{announce},
                cpr::Parameters {
                        {"info_hash", tf.infoHash},
                        {"peer_id", peerId},
                        {"port", std::to_string(port)},
                        {"uploaded", std::to_string(0)},
                        {"downloaded", std::to_string(0)},
                        {"left", std::to_string(tf.length)},
                        {"compact", std::to_string(1)}
                },
                cpr::Timeout{10000/*20000*/}
        );
        if (res.status_code != 200 || res.text.find("fail") != std::string::npos) {
            // std::cerr << res.status_code << std::endl;
            std::cout << "Error in connecting!" << std::endl;
            continue;
        }
        std::cout << "Successfully connected to tracker!" << std::endl;

        std::string data = (std::string)res.text;
        size_t beg = data.find("peers") + 5;
        size_t count = getNumWithIncrementIdx(data, beg);
        const size_t countOfBytes = 6;
        for (size_t idx = beg; idx <= beg + count; idx += countOfBytes){
            peers_.push_back(parsePeer(data.substr(idx, countOfBytes)));
        }
    }
    if (peers_.empty()){
        throw std::runtime_error("No peers found!");
    }
    return;
}

const std::vector<Peer> &TorrentTracker::GetPeers() const {
    return peers_;
}
