#pragma once

// Window Library
#include <windows.h>

// D3D Library
#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

#include <d3d11.h>
#include <d3dcompiler.h>

// Standard Library
#include <cmath>
#include <map>
#include <vector>
#include <string>
#include <chrono>
#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <functional>
#include <filesystem>

#include <cassert>

#include "Global/Constant.h"
#include "Global/Enum.h"
#include "Global/Struct.h"
#include "Global/Macro.h"
#include "Global/Function.h"

template <typename T>
using TArray = std::vector<T>;

template <typename K, typename V>
using TMap = std::map<K, V>;

using FString = std::string;
using std::clamp;
using std::unordered_map;
using std::to_string;
using std::function;
using std::wstring;
using std::cout;
using std::endl;

// File System
namespace filesystem = std::filesystem;
using filesystem::path;
using filesystem::exists;
using filesystem::create_directories;
