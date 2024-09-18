#include "byte_tools.h"
#include <openssl/sha.h>
#include <vector>
#include "bencode.h"
using namespace Bencode;

size_t BytesToInt(std::string_view bytes) {
    size_t ret = 0;
    for (int i = 0; i < bytes.size(); i++){
        ret <<= 8;
        ret += reinterpret_cast<unsigned char&>(const_cast<char&>(bytes[i]));
    }
    return ret;
}

template<typename T>
std::string IntToBytes(T number, bool isReversed) {
    assert(4 == sizeof(T));
    std::string ans = "";
    char *pointer = reinterpret_cast<char *>(&number);
    for (int i = 3; i >= 0; i--) {
        ans += pointer[i];
    }
    if (isReversed){
        std::reverse(ans.begin(), ans.end());
    }
    return ans;
}

template std::string IntToBytes<int>(int, bool);
template std::string IntToBytes<unsigned int>(unsigned int, bool);


std::string CalculateSHA1(const std::string& msg) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1((unsigned char*)(msg.c_str()), msg.size(), hash);
    std::string SHA1_string = "";
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        SHA1_string += hash[i];
    }
    return SHA1_string;
}



std::string HexEncode(const std::string& input) {
    std::string back = "";
    for (const char& c : input){
        if (c >= '0' && c <= '9' || c >= 'A' && c <= 'F' || c >= 'a' && c <= 'f'){
            back += c;
        }
    }
    return back;
}

std::string RandomString(size_t length) {
    std::random_device random;
    std::string result;
    result.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        result.push_back(random() % ('Z' - 'A') + 'A');
    }
    return result;
}