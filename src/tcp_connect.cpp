#include "tcp_connect.h"
#include "byte_tools.h"
#include "bencode.h"


#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdexcept>
#include <cstring>
#include <iostream>
#include <chrono>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <limits>
#include <utility>
#include <cassert>



TcpConnect::TcpConnect(std::string ip, int port, std::chrono::milliseconds connectTimeout,
                       std::chrono::milliseconds readTimeout) : ip_(std::move(ip)), port_(port),
                                                                connectTimeout_(connectTimeout),
                                                                readTimeout_(readTimeout), sock_(-1) {}

TcpConnect::~TcpConnect() {
    CloseConnection();
}


void TcpConnect::EstablishConnection() {
    // Создаем сокет
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        throw std::runtime_error(std::string("Failed to create socket: ") + std::strerror(errno));
    }
    // Включаем неблокирующий режим работы
    if (fcntl(sock, F_SETFL, O_NONBLOCK) < 0) {
        throw std::runtime_error(std::string("Failed to set socket to non-blocking mode: ") + std::strerror(errno));
    }

    // Задаем адрес сервера
    // assert(t1 == t2);
    sockaddr_in _sockaddr = {AF_INET, htons(port_), BytesToInt(Bencode::getCompactIp(ip_, true))};
    // sockaddr_in _sockaddr = {AF_INET, htons(port_), inet_addr(ip_.c_str())};
    

    // Подключаемся к серверу
    assert(sizeof(sockaddr) == sizeof(sockaddr_in)); // for normal reinterpret_cast
    int result = connect(sock,  reinterpret_cast<sockaddr*>(&_sockaddr), sizeof(sockaddr));
    if (result < 0 && errno != EINPROGRESS) {
        throw std::runtime_error("Error in setting up a connection! Error:\t" + errno);
    }

    pollfd _pollfd = {sock, POLLOUT, 0};
    auto start = std::chrono::steady_clock::now();
    while (!(_pollfd.revents & POLLOUT)) {
        auto now = std::chrono::steady_clock::now();
        auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
        if (delta > connectTimeout_) {
            throw std::runtime_error("Connection timed out!");
        }
        result = poll(&_pollfd, 1, connectTimeout_.count() - delta.count());
        if (result < 0){
            throw std::runtime_error("Error in poll (in 'EstablishConnectin')! Error:\t" + errno);
        }
        else if (result == 0){
            throw std::runtime_error("Poll (in 'EstablishConnectin') timed out!");
        }
    }

    // Выключаем неблокирующий режим работы
    if (fcntl(sock, F_SETFL, 0) < 0) {
        throw std::runtime_error("Error in fcntl! Error:\t" + errno);
    }

    sock_ = sock;
    return;
}


/*
 * Послать данные в сокет
 * Полезная информация:
 * - https://man7.org/linux/man-pages/man2/send.2.html
 */
void TcpConnect::SendData(const std::string& data) const {
    if (send(sock_, data.c_str(), data.size(), MSG_NOSIGNAL) < 0) {
        throw std::runtime_error("Error in sending data!");
    }
}


/*
 * Прочитать данные из сокета.
 * Если передан `bufferSize`, то прочитать `bufferSize` байт.
 * Если параметр `bufferSize` не передан, то сначала прочитать 4 байта, а затем прочитать количество байт, равное
 * прочитанному значению.
 * Первые 4 байта (в которых хранится длина сообщения) интерпретируются как целое число в формате big endian,
 * см https://wiki.theory.org/BitTorrentSpecification#Data_Types
 * Полезная информация:
 * - https://man7.org/linux/man-pages/man2/poll.2.html
 * - https://man7.org/linux/man-pages/man2/recv.2.html
 */
std::string TcpConnect::ReceiveData(size_t bufferSize) const {
    std::string buffer;
    if (bufferSize == 0) {
        // Прочитать длину сообщения
        std::string lenBuffer(4, 0);
        size_t bytesReceived = 0;
        while (bytesReceived < 4) {
            pollfd _pollfd = {sock_, POLLIN, 0};
            int result = poll(&_pollfd, 1, readTimeout_.count());
            if (result < 0){
                throw std::runtime_error("Error in poll (in 'ReceiveData')! Error:\t" + errno);
            }
            else if (result == 0){
                throw std::runtime_error("Poll (in 'ReceiveData') timed out!");
            }
            else if (_pollfd.revents & POLLIN) {
                ssize_t bytesRead = recv(sock_, &lenBuffer[bytesReceived], 4 - bytesReceived, 0);
                if (bytesRead <= 0){
                    throw std::runtime_error("Error in recv (in 'ReceiveData')!");
                }
                bytesReceived += bytesRead;
            }
        }
        bufferSize = BytesToInt(lenBuffer);
    } 
    buffer.resize(bufferSize);
    size_t bytesReceived = 0;
    while (bytesReceived < bufferSize) {
        pollfd _pollfd = {sock_, POLLIN, 0};
        int result = poll(&_pollfd, 1, readTimeout_.count());
        if (result < 0){
            throw std::runtime_error("Error in poll (in 'ReceiveData')! Error:\t" + errno);
        }
        else if (result == 0){
            throw std::runtime_error("Poll (in 'ReceiveData') timed out!");
        }
        else if (_pollfd.revents & POLLIN) {
            ssize_t bytesRead = recv(sock_, &buffer[bytesReceived], bufferSize - bytesReceived, 0);
            if (bytesRead <= 0){
                throw std::runtime_error("Error in recv (in 'ReceiveData')!");
            }
            bytesReceived += bytesRead;
        }
    }
    return buffer;
}


/*
 * Закрыть сокет
 */
void TcpConnect::CloseConnection() {
    if (sock_ >= 0) {
        if (close(sock_) < 0) {
            throw std::runtime_error("Error in closing socket! Error:\t" + errno);
        }
        sock_ = -1;
    }
}

const std::string &TcpConnect::GetIp() const {
    return ip_;
}

int TcpConnect::GetPort() const {
    return port_;
}