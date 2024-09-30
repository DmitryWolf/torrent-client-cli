#include "torrent_tracker.h"
#include "piece_storage.h"
#include "peer_connect.h"
#include "byte_tools.h"
#include <iostream>
#include <cassert>
#include <cmath>
#include <filesystem>
#include <algorithm>
#include "StaticThreadPool.h"

namespace fs = std::filesystem;
std::mutex cerrMutex, coutMutex;


// Готовим директорию для скачивания
std::filesystem::path PrepareDownloadDirectory(const std::string& saveDirectory) {
    std::filesystem::path outputDirectory = saveDirectory;
    std::filesystem::create_directories(outputDirectory);
    return outputDirectory;
}

// Удалить временный файл
void DeleteDownloadedFile(const std::filesystem::path& outputFilename) {
    std::filesystem::remove(outputFilename);
}


// Запуск многопоточного скачивания
bool RunDownloadMultithread(PieceStorage& pieces, const TorrentFile& torrentFile, const std::string& ourId, const TorrentTracker& tracker, const size_t countOfPiecesToDownload) {
    using namespace std::chrono_literals;
    std::vector<std::shared_ptr<PeerConnect>> peerConnections;

    std::cerr << "WE NEED TO DOWNLOAD " << countOfPiecesToDownload << std::endl;
    for (const Peer& peer : tracker.GetPeers()) {
        peerConnections.emplace_back(std::make_shared<PeerConnect>(peer, torrentFile, ourId, pieces));
    }

    
    size_t workersCount = std::min(static_cast<size_t>(std::thread::hardware_concurrency()), peerConnections.size());
    StaticThreadPool peerThreads(workersCount);

    for (auto& peerConnectPtr : peerConnections){
        peerThreads.Submit(
            [peerConnectPtr] () {
                bool tryAgain = true;
                int attempts = 0;
                do {
                    try {
                        ++attempts;
                        peerConnectPtr->Run();
                    } catch (const std::runtime_error& e) {
                        // std::lock_guard<std::mutex> cerrLock(cerrMutex);
                        // std::cerr << "Runtime error: " << e.what() << std::endl;
                    } catch (const std::exception& e) {
                        // std::lock_guard<std::mutex> cerrLock(cerrMutex);
                        // std::cerr << "Exception: " << e.what() << std::endl;
                    } catch (...) {
                        // std::lock_guard<std::mutex> cerrLock(cerrMutex);
                        // std::cerr << "Unknown error" << std::endl;
                    }
                    tryAgain = peerConnectPtr->Failed() && attempts < 3;
                } while (tryAgain);
            }
        );
    }

    {
        std::lock_guard<std::mutex> coutLock(coutMutex);
        std::cout << "Started " << workersCount << " threads for peers" << std::endl;
    }
    std::this_thread::sleep_for(10s);
    while (pieces.PiecesSavedToDiscCount() < countOfPiecesToDownload) {
        if (pieces.PiecesInProgressCount() == 0) {
            {
                std::lock_guard<std::mutex> coutLock(coutMutex);
                std::cout
                        << "Want to download more pieces but all peer connections are not working. Let's request new peers"
                        << std::endl;
            }

            for (auto& peerConnectPtr : peerConnections) {
                peerConnectPtr->Terminate();
            }
            peerThreads.Join();
            return true;
        }
        std::this_thread::sleep_for(1s);
    }

    {
        std::lock_guard<std::mutex> coutLock(coutMutex);
        std::cout << "Terminating all peer connections" << std::endl;
    }
    for (auto& peerConnectPtr : peerConnections) {
        peerConnectPtr->Terminate();
    }

    peerThreads.Join();

    return false;
}

// Подготовка к скачиванию + запуск многопоточной загрузки
void DownloadTorrentFile(const TorrentFile& torrentFile, PieceStorage& pieces, const std::string& ourId, const size_t countOfPiecesToDownload) {
    
    TorrentTracker tracker(torrentFile.announce_list);
    bool requestMorePeers = false;
    do {
        int attempts = 0;
        bool gotPeers = false;
        do {
            try {
                tracker.UpdatePeers(torrentFile, ourId, 12345);
                gotPeers = true;
            } catch (...) {
                attempts++;
                if (attempts > 3) {
                    throw std::runtime_error("Error in updating peers!");
                }
            }
        } while (!gotPeers);

        if (tracker.GetPeers().empty()) {
            std::cerr << "No peers found. Cannot download a file" << std::endl;
        }

        std::cout << "Found " << tracker.GetPeers().size() << " peers" << std::endl;
        for (const Peer& peer : tracker.GetPeers()) {
            std::cout << "Found peer " << peer.ip << ":" << peer.port << std::endl;
        }

        requestMorePeers = RunDownloadMultithread(pieces, torrentFile, ourId, tracker, countOfPiecesToDownload);
    } while (requestMorePeers);
}



// рекурсивно создаем директории по указанному пути
void create_directories_for_file(const std::filesystem::path& file_path) {
    // Получаем путь к директории, содержащей файл
    std::filesystem::path dir_path = file_path.parent_path();
    
    // Создаем все необходимые директории
    try {
        std::filesystem::create_directories(dir_path);
        std::cout << "Directories created successfully." << std::endl;
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error creating directories: " << e.what() << std::endl;
    }
}

// Распределяем байты из одного общего файла по файлам из TorrentFile
void DistributePiecesBetweenFiles(const TorrentFile& tf, const std::string& fileName, const std::string& saveDirectory) {
    std::ifstream inputFile(fileName, std::ios::in | std::ios::binary);
    char ch;
    std::cout << std::endl << std::endl;
    for (const auto& it : tf.files) {
        std::string road = saveDirectory + "/";
        if (tf.name == "") {
            road += it.path;
        } else {
            road += tf.name + "/" + it.path;
        }
        create_directories_for_file(road);

        std::ofstream outputFile(road, std::ios::out | std::ios::binary);
        if (!outputFile.is_open()) {
            std::cerr << "Error in opening file: " << road << std::endl;
            return;
        }
        for (size_t i = 0; i < it.length; ++i) {
            inputFile.get(ch);
            outputFile.put(ch);
        }
        outputFile.close();
    }
    inputFile.close();
}

void RunAllStagesOfDownloadingTorrentFile(const std::string& saveDirectory, size_t percent, const std::string& torrentFilePath) {
    std::cout << "\n\n\nСкачивание " << percent << "% файла " << torrentFilePath << " в директорию " << saveDirectory << std::endl;
    TorrentFile torrentFile;
    try {
        torrentFile = LoadTorrentFile(torrentFilePath);
        std::cout << "Loaded torrent file " << torrentFilePath << ". " << torrentFile.comment << std::endl;
    } catch (const std::invalid_argument& e) {
        std::cerr << e.what() << std::endl;
        return;
    }

    const std::string PeerId = "TESTAPPDONTWORRY" + RandomString(4);

    const std::filesystem::path outputDirectory = PrepareDownloadDirectory(saveDirectory);
    
    std::string fileName = (outputDirectory / RandomString(40)).string();

    PieceStorage pieces(torrentFile, outputDirectory, fileName);
    size_t countOfPiecesToDownload = std::ceil(((static_cast<long double>(percent) / 100) * torrentFile.pieceHashes.size()));
    std::cerr << torrentFile.name << std::endl;
    pieces.SetNewSize(countOfPiecesToDownload);

    DownloadTorrentFile(torrentFile, pieces, PeerId, countOfPiecesToDownload);
    if (percent == 100) {
        std::cout << "Distributing files..." << std::endl; 
        DistributePiecesBetweenFiles(torrentFile, fileName, saveDirectory);
    }
    DeleteDownloadedFile(fileName);
}


int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " -d <save_directory> <torrent_file_path>" << std::endl;
        return 1;
    }

    std::string saveDirectory;
    std::string torrentFilePath;
    int percent = 100;

    if (std::string(argv[1]) == "-d") {
        saveDirectory = argv[2];
        torrentFilePath = argv[3];
    } else {
        std::cerr << "Invalid arguments. Usage: " << argv[0] << " -d <save_directory> <torrent_file_path>" << std::endl;
        return 1;
    }

    if (saveDirectory.empty() || torrentFilePath.empty()) {
        std::cerr << "Invalid arguments. Please check and try again." << std::endl;
        return 1;
    }

    saveDirectory = fs::absolute(saveDirectory).string();
    torrentFilePath = fs::absolute(torrentFilePath).string();

    RunAllStagesOfDownloadingTorrentFile(saveDirectory, percent, torrentFilePath);
    return 0;
}