#pragma once

#include <string>
#include <random>
#include <assert.h>
/*
 * Преобразовать 4 байта в формате big endian в int
 */
size_t BytesToInt(std::string_view bytes);

//Integer в bigend
template<typename T>
std::string IntToBytes(T n, bool isReversed = false);
/*
 * Расчет SHA1 хеш-суммы. Здесь в результате подразумевается не человеко-читаемая строка, а массив из 20 байтов
 * в том виде, в котором его генерирует библиотека OpenSSL
 */
std::string CalculateSHA1(const std::string& msg);

/*
 * Представить массив байтов в виде строки, содержащей только символы, соответствующие цифрам в шестнадцатеричном исчислении.
 * Конкретный формат выходной строки не важен. Важно то, чтобы выходная строка не содержала символов, которые нельзя
 * было бы представить в кодировке utf-8. Данная функция будет использована для вывода SHA1 хеш-суммы в лог.
 */
std::string HexEncode(const std::string& input);

std::string RandomString(size_t length);