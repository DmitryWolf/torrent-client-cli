#include "piece_storage.h"
#include "byte_tools.h"
#include <iostream>


/*
-------------------------------------------------------------
------------------------PieceStorage-------------------------
-------------------------------------------------------------
*/

PieceStorage::PieceStorage(const TorrentFile& tf, const std::filesystem::path& outputDirectory, const std::string& fileName) {
    size_t tailSize = 0;
    for (const auto& it : tf.files) {
        tailSize += it.length;
    }
    assert(tf.length >= tailSize && tf.pieceLength >= tf.length - tailSize);
    tailSize = tf.pieceLength - (tf.length - tailSize);

    for (size_t i = 0; i < tf.pieceHashes.size(); ++i) {
        size_t pieceLength;
        if (i != tf.pieceHashes.size() - 1){
            pieceLength = tf.pieceLength;
        } else {
            if (tf.length % tf.pieceLength == 0){
                pieceLength = tf.pieceLength;
            } else{
                pieceLength = tf.length % tf.pieceLength;
            }
        }
        if (i == tf.pieceHashes.size() - 1) {
            pieceLength = tailSize;
        }

        auto piece = std::make_shared<Piece>(i, pieceLength, tf.pieceHashes[i]);
        remainPieces_.push_back(piece);
    } 
    totalSize_ = remainPieces_.size();
    piecesInProgressCount_ = 0;
    pieceLength_ = tf.pieceLength;
    
    fileName_ = fileName;
    if (!std::filesystem::exists(outputDirectory)) {
        std::filesystem::create_directories(outputDirectory);
    }

    // reserve tf.length bytes
    std::ofstream file(fileName_, std::ios::binary | std::ios::out);
    file.seekp(tf.length - 1);
    file.put('\0');
    file.close();

    OpenFile();
}

PieceStorage::~PieceStorage() {
    CloseFile();
}

int PieceStorage::OpenFile() {
    fd_ = open(fileName_.c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd_ == -1) {
        throw std::runtime_error("Error opening file");
    }
    return fd_;
}

void PieceStorage::CloseFile() {
    if (fd_ != -1) {
        close(fd_);
        fd_ = -1;
    }
}

PiecePtr PieceStorage::GetNextPieceToDownload() {
    std::unique_lock lock(mutex_);
    if (remainPieces_.empty()){
        throw std::runtime_error("Queue is empty!");
    }
    PiecePtr toDownload = remainPieces_.front();
    remainPieces_.pop_front();
    downloadingPieces_[toDownload->GetIndex()] = toDownload;
    ++piecesInProgressCount_;
    return toDownload;
}

void PieceStorage::PieceProcessed(const PiecePtr& piece) {
    --piecesInProgressCount_;
    std::unique_lock lock(mutex_);
    SavePieceToDisk(piece);
}

bool PieceStorage::QueueIsEmpty() const {
    std::unique_lock lock(mutex_);
    return remainPieces_.empty();
}

size_t PieceStorage::PiecesSavedToDiscCount() const {
    std::unique_lock lock(mutex_);
    return indicesOfSavedPiecesToDisc_.size();
}

size_t PieceStorage::TotalPiecesCount() const {
    std::unique_lock lock(mutex_);
    return totalSize_;
}

void PieceStorage::CloseOutputFile() {
    std::unique_lock lock(mutex_);
    CloseFile();
}

const std::vector<size_t>& PieceStorage::GetPiecesSavedToDiscIndices() const {
    std::unique_lock lock(mutex_);
    return indicesOfSavedPiecesToDisc_;
}

size_t PieceStorage::PiecesInProgressCount() const {
    std::unique_lock lock(mutex_);
    return piecesInProgressCount_.load();
}

void PieceStorage::SavePieceToDisk(const PiecePtr& piece) {
    size_t pieceIndex = piece->GetIndex();
    std::string data = piece->GetData();
    size_t offset = pieceIndex * pieceLength_;
    
    if (fd_ == -1) {
        OpenFile();
    }

    // Запись данных в файл с помощью функции write в цикле
    for (size_t i = 0; i < data.size(); i += SSIZE_MAX) {
        // Установка позиции в файле с помощью функции lseek
        off_t pos = lseek(fd_, offset + i, SEEK_SET);
        if (pos == -1) {
            std::cerr << "Ошибка установки позиции в файле" << std::endl;
            CloseFile();
            return;
        }

        // Запись блока данных в файл с помощью функции write
        ssize_t bytes_written = write(fd_, data.c_str() + i, std::min(static_cast<size_t>(SSIZE_MAX), data.size() - i));
        if (bytes_written == -1) {
            std::cerr << "Ошибка записи в файл" << std::endl;
            CloseFile();
            return;
        }
    }
    downloadingPieces_.erase(pieceIndex);
    indicesOfSavedPiecesToDisc_.push_back(pieceIndex);
    std::cerr << "SAVED " << pieceIndex << " , скачивается " << downloadingPieces_.size() << " , осталось: " << remainPieces_.size() << std::endl;
}

void PieceStorage::DecrementPieceInProgressCounter() {
    piecesInProgressCount_.fetch_sub(1);
}

void PieceStorage::BackPieceToQueue(const size_t pieceIndex) {
    std::unique_lock lock(mutex_);
    if (downloadingPieces_.contains(pieceIndex)){
        remainPieces_.push_back(downloadingPieces_[pieceIndex]);
        downloadingPieces_.erase(pieceIndex);
    }
}


void PieceStorage::SetNewSize(const size_t newSize) {
    std::unique_lock lock(mutex_); // just in case
    size_t oldSize = remainPieces_.size();
    for (size_t i = newSize; i < oldSize; i++){
        remainPieces_.pop_back();
    }
}