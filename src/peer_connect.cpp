#include "byte_tools.h"
#include "peer_connect.h"
#include "message.h"
#include <iostream>
#include <sstream>
#include <utility>
#include <cassert>

using namespace std::chrono_literals;
#define BYTESIZE 8;


/*
-------------------------------------------------------------
--------------------PeerPiecesAvailability-------------------
-------------------------------------------------------------
*/


PeerPiecesAvailability::PeerPiecesAvailability() :
    bitfield_("") {}

PeerPiecesAvailability::PeerPiecesAvailability(std::string bitfield) :
    bitfield_(std::move(bitfield)) {}

bool PeerPiecesAvailability::IsPieceAvailable(size_t pieceIndex) const {
    if (pieceIndex > Size()){
        throw std::out_of_range("Index out of range!");
    }
    size_t byteIndex = pieceIndex / BYTESIZE;
    size_t bitIndex = pieceIndex % BYTESIZE;
    char byte = bitfield_[byteIndex];
    bool bit = (byte >> bitIndex) & 1;
    return bit;
}


void PeerPiecesAvailability::SetPieceAvailability(size_t pieceIndex) {
    if (pieceIndex > Size()){
        throw std::out_of_range("Index out of range!");
    }
    size_t byteIndex = pieceIndex / BYTESIZE;
    size_t bitIndex = pieceIndex % BYTESIZE;
    char byte = bitfield_[byteIndex];
    byte |= ((byte >> bitIndex) | 1);
    bitfield_[byteIndex] = byte;
}

size_t PeerPiecesAvailability::Size() const {
    return bitfield_.size() * BYTESIZE;
}




/*
-------------------------------------------------------------
-------------------------PEERCONNECT-------------------------
-------------------------------------------------------------
*/


PeerConnect::PeerConnect(const Peer& peer, const TorrentFile& tf, std::string selfPeerId, PieceStorage& pieceStorage) :
    socket_(TcpConnect(peer.ip, peer.port, 1000ms, 1000ms)), selfPeerId_(std::move(selfPeerId)), tf_(tf),
    pieceStorage_(pieceStorage), terminated_(false), choked_(true), pendingBlock_(false), failed_(false),
    pieceInProgress_(nullptr) {}

void PeerConnect::Run() {
    while(!terminated_.load()){
        if (EstablishConnection()) {
            // std::cout << "Connection established to peer" << std::endl;
            MainLoop();
        } else {
            // std::cerr << "Cannot establish connection to peer" << std::endl;
            Terminate();
        }
    }
}

void PeerConnect::Terminate() {
    terminated_.store(true);
    // std::cerr << "Terminate" << std::endl;
}

bool PeerConnect::Failed() const {
    return failed_;
}


void PeerConnect::PerformHandshake() {
    socket_.EstablishConnection();

    std::string handshakeMessage;
    handshakeMessage.reserve(68);

    handshakeMessage += static_cast<char>(19); // 1 - pstrlen
    handshakeMessage += "BitTorrent protocol"; // 19 - pstr
    handshakeMessage += std::string(8, 0); // 8 - reserved (0)
    handshakeMessage += tf_.infoHash; // 20 - info_hash
    handshakeMessage += selfPeerId_; // 20 - peer_id

    socket_.SendData(handshakeMessage);

    std::string ans = socket_.ReceiveData(handshakeMessage.size());

    if (static_cast<int>(ans[0]) != 19 || ans.substr(1, 19) != "BitTorrent protocol") {
        throw std::runtime_error("Failed handshake!");
    }
}

void PeerConnect::ReceiveBitfield() {
    Message receivedMessage = Message::Parse(socket_.ReceiveData());
    if (receivedMessage.id == MessageId::Unchoke){
        choked_ = false;
    }
    else if (receivedMessage.id == MessageId::BitField){
        piecesAvailability_ = PeerPiecesAvailability(receivedMessage.payload);
    }
}

void PeerConnect::SendInterested() {
    std::string sendMessage = Message::Init(MessageId::Interested, "").ToString();
    socket_.SendData(sendMessage);
}

bool PeerConnect::EstablishConnection() {
    try {
        PerformHandshake();
        ReceiveBitfield();
        SendInterested();
        return true;
    } catch (const std::exception& e) {
        // std::cerr << "Failed to establish connection with peer " << socket_.GetIp() << ":" <<
        //     socket_.GetPort() << " -- " << e.what() << std::endl;
        return false;
    }
}


void PeerConnect::RequestPiece() {
    // Проверяем, есть ли уже часть в процессе загрузки
    if (!pieceInProgress_) {
        // Если нет, получаем следующую часть для скачивания
        pieceInProgress_ = pieceStorage_.GetNextPieceToDownload();
        isPieceDownloadingNow_ = true;
    }

    // Получаем первый недостающий блок в части
    Block* block;
    {
        // std::unique_lock lock(mutex_);
        block = pieceInProgress_->FirstMissingBlock();
    } 
    
    // Если блок найден
    if (block) {
        // Создаем запрос
        std::string request; 
        // 17 байт: 4 байта префикс длины + 1 байт ID сообщения + 12 байт полезные данные
        request.reserve(17);

        request += IntToBytes(htonl(13), true); // 4 байта - длина сообещния (13)
        request += static_cast<char>(6); // 1 байт - код (6)
        request += IntToBytes(htonl(block->piece), true); // 4 байта - индекс куска
        request += IntToBytes(htonl(block->offset), true); // 4 байта - начальная позиция куска
        request += IntToBytes(htonl(block->length), true); // 4 байта - длина куска

        // Отправляем запрос через сокет
        socket_.SendData(request);

        // Устанавливаем флаг, что запрос на блок отправлен
        pendingBlock_ = true;
    }
}

void PeerConnect::MainLoop() {
    clearFlags();
    while (!terminated_.load()){
        std::string message;
        try{
             message = socket_.ReceiveData();
        }
        catch(...){
            // if (isPieceDownloadingNow_ && pieceInProgress_.get()->GetIndex() == pieceStorage_.TotalPiecesCount() - 1) {
            //     pieceStorage_.PieceProcessed(pieceInProgress_);
            //     isPieceDownloadingNow_ = false;
            //     pieceInProgress_.reset();
            // }
            break;
        }
        if (message.empty()){
            failed_ = true;
            break;
        }

        MessageId messageId = static_cast<MessageId>(message[0]);
        switch (messageId){
            case MessageId::Choke:{
                choked_ = true;
                failed_ = true;
                }
                break;
            case MessageId::Unchoke:{
                choked_ = false;
                }
                break;
            case MessageId::Have:{
                size_t pieceIndex = BytesToInt(message.substr(1, 4));
                piecesAvailability_.SetPieceAvailability(pieceIndex);
                }
                break;
            case MessageId::Piece:{
                size_t offset = BytesToInt(message.substr(5, 4));
                offset /= (1 << 14);

                std::string data = message.substr(9);

                if (!pieceInProgress_){ // Может ли нам вообще прийти MessageId::Piece до того, как мы сделаем запрос?
                    pieceInProgress_ = pieceStorage_.GetNextPieceToDownload();
                    isPieceDownloadingNow_ = true;
                }
                pieceInProgress_->SaveBlock(offset, data);
                pendingBlock_ = false;

                if (pieceInProgress_->AllBlocksRetrieved()) {
                    pieceStorage_.PieceProcessed(pieceInProgress_);
                    isPieceDownloadingNow_ = false;
                    pieceInProgress_.reset();
                }
                }
                break;
            default:
                break;
        }
        if (!choked_ && !pendingBlock_ 
        && (!pieceStorage_.QueueIsEmpty() || isPieceDownloadingNow_)){
            RequestPiece();
        }
    }
    if (isPieceDownloadingNow_){
        std::unique_lock lock(mutex_);
        std::cout << "ВЕРНУЛИ ЧАСТЬ НОМЕР " << pieceInProgress_->GetIndex();
        pieceInProgress_->Reset();
        pieceStorage_.DecrementPieceInProgressCounter();
        pieceStorage_.BackPieceToQueue(pieceInProgress_->GetIndex());
        isPieceDownloadingNow_ = false;
    }

}


void PeerConnect::clearFlags() {
    failed_ = false;
    pendingBlock_ = false;
    pieceInProgress_ = nullptr;
    choked_ = true;
    // terminated_(false), choked_(true), pendingBlock_(false), failed_(false),
    // pieceInProgress_(nullptr)
}