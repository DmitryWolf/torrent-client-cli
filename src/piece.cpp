#include "byte_tools.h"
#include "piece.h"
#include <iostream>
#include <algorithm>

namespace {
constexpr size_t BLOCK_SIZE = 1 << 14;
}

Piece::Piece(size_t index, size_t length, std::string hash) :
    index_(index), length_(length), hash_(std::move(hash)) {
    
    size_t numBlocks = (length + BLOCK_SIZE - 1) / BLOCK_SIZE;
    for (size_t i = 0; i < numBlocks; ++i) {
        uint32_t blockLength = std::min(BLOCK_SIZE, length - i * BLOCK_SIZE);
        blocks_.emplace_back(Block{static_cast<uint32_t>(index), static_cast<uint32_t>(i * BLOCK_SIZE), blockLength, Block::Missing, ""});
    }
}

bool Piece::HashMatches() const {
    std::unique_lock lock(mutex_);
    return GetHash() == hash_;
}

Block* Piece::FirstMissingBlock(){
    std::unique_lock lock(mutex_);
    for (auto& block : blocks_){
        if (block.status == Block::Missing){
            block.status = Block::Pending;
            return &block;
        }
    }
    return nullptr;
}

size_t Piece::GetIndex() const {
    return index_.load();
}


void Piece::SaveBlock(size_t blockOffset, std::string data) {
    std::unique_lock lock(mutex_);
    if (blockOffset >= blocks_.size()) {
        throw std::out_of_range("Block offset out of range!");
    }
    Block& block = blocks_[blockOffset];
    block.data = std::move(data);
    block.status = Block::Retrieved;
}



bool Piece::AllBlocksRetrieved() const {
    std::unique_lock lock(mutex_);
    return std::all_of(blocks_.begin(), blocks_.end(), [](const Block& block) {
        return block.status == Block::Retrieved;
    });
}


std::string Piece::GetData() const {
    std::unique_lock lock(mutex_);
    std::string data;
    data.reserve(length_.load());
    for (const auto& block : blocks_) {
        data.append(block.data);
    }
    return data;
}


std::string Piece::GetDataHash() const {
    return CalculateSHA1(GetData());
}


const std::string& Piece::GetHash() const {
    return hash_;
}


void Piece::Reset(){
    std::unique_lock lock(mutex_);
    for (auto& block : blocks_) {
        block.status = Block::Missing;
        block.data.clear();
    }
}