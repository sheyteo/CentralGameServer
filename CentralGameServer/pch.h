#pragma once
#include <stdint.h>

#include <WinSDKVer.h>
#define _WIN32_WINNT 0x0A00
#include <SDKDDKVer.h>

#include <concepts>
#include <iostream>
#include <vector>
#include <queue>
#include <map>
#include <chrono>
#include <shared_mutex>
#include <list>
#include <regex>
#include <fstream>

#include <algorithm>
#include <random>

#include "asio/include/asio.hpp"
#include "nlohmann/json.hpp"

using namespace std::chrono_literals;

template<typename T>
concept Indexable = requires {
    { std::declval<T>()[0] };
};

template<typename T>
concept SimpleType = std::is_arithmetic_v<T>;

template<typename T>
concept ComplexType = std::conjunction_v <std::negation<std::is_arithmetic<T>>>;