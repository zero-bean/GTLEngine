#pragma once

#define NOMINMAX

// Window Library
#include <windows.h>

// D3D Library
#include <d3d11.h>
#include <d3dcompiler.h>

// Standard Library
#include <cmath>
#include <cassert>
#include <string>
#include <chrono>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <functional>
#include <filesystem>
#include <iterator>

// Global Included
#include "Global/Constant.h"
#include "Global/Enum.h"
#include "Global/Matrix.h"
#include "Global/Vector.h"
#include "Global/CoreTypes.h"
#include "Global/Macro.h"
#include "Global/Function.h"
#include "Global/Memory.h"

//STL Redefine
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <list>
#include <queue>
#include <stack>
template<typename T, typename Alloc = std::allocator<T>>
using TArray = std::vector<T, Alloc>;
template<typename T, typename Alloc = std::allocator<T>>
using TLinkedList = std::list<T, Alloc>;
template<typename T, typename Alloc = std::allocator<T>>
using TDoubleLinkedList = std::list<T, Alloc>;
template<typename T, typename Hash = std::hash<T>, typename Eq = std::equal_to<T>, typename Alloc = std::allocator<T>>
using TSet = std::unordered_set<T, Hash, Eq, Alloc>;
template<typename KeyType, typename ValueType, typename Hash = std::hash<KeyType>, typename Eq = std::equal_to<KeyType>, typename Alloc = std::allocator<std::pair<const KeyType, ValueType>>>
using TMap = std::unordered_map<KeyType, ValueType, Hash, Eq, Alloc>;
template<typename T1, typename T2>
using TPair = std::pair<T1, T2>;
template<typename T, size_t N>
using TStaticArray = std::array<T, N>;
template<typename T, typename Container = std::deque<T>>
using TQueue = std::queue<T, Container>;

using FString = std::string;
using std::clamp;
using std::unordered_map;
using std::to_string;
using std::function;
using std::wstring;
using std::cout;
using std::cerr;
using std::min;
using std::max;
using std::exception;
using std::stoul;
using std::ofstream;
using std::ifstream;
using std::setw;
using std::sort;
using std::shared_ptr;
using std::unique_ptr;
using std::streamsize;

// File System
namespace filesystem = std::filesystem;
using filesystem::path;
using filesystem::exists;
using filesystem::create_directories;

// Library Linking
#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

#define IMGUI_DEFINE_MATH_OPERATORS
#include "Render/UI/Window/Public/ConsoleWindow.h"
