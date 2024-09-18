#include "message.h"
#include "byte_tools.h"


Message Message::Parse(const std::string& messageString) {
    if (messageString.empty()){
        return {MessageId::KeepAlive, messageString.size(), messageString};
    }
    return {static_cast<MessageId>(messageString[0]), messageString.size(), messageString.substr(1, messageString.size() - 1)};
}

Message Message::Init(MessageId id, const std::string& payload) {
    return {id, payload.size(), payload};
}

std::string Message::ToString() const {
    // 4 байта + id + payload
    std::string messageString = IntToBytes(static_cast<int>(messageLength) + 1);
    if (id == MessageId::KeepAlive) {
        return messageString;
    }
    messageString += static_cast<char>(id);
    messageString += payload;
    return messageString;

}