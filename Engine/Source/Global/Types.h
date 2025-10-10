#pragma once

//STL Redefine
#include <list>
#include <optional>
#include <queue>
#include <deque>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <functional>

template<typename T, typename AllocatorType = std::allocator<T>>
class TArray : public std::vector<T, AllocatorType>
{
public:
	using std::vector<T, AllocatorType>::vector; /** 생성자 상속 */

    /** 요소 추가 */
    int Add(const T& Item)
    {
        this->push_back(Item);
        return static_cast<int>(this->size() - 1);
    }

    template<typename... Args>
    int Emplace(Args&&... args)
    {
        this->emplace_back(std::forward<Args>(args)...);
        return static_cast<int>(this->size() - 1);
    }

    /** 고유 요소만 추가 */
    int AddUnique(const T& Item)
    {
        auto it = std::find(this->begin(), this->end(), Item);
        if (it == this->end())
        {
            return Add(Item);
        }
        return static_cast<int>(std::distance(this->begin(), it));
    }

    /** 배열 병합 */
    void Append(const TArray<T>& Other)
    {
        this->insert(this->end(), Other.begin(), Other.end());
    }

    /** 삽입 */
    void Insert(const T& Item, int Index)
    {
        this->insert(this->begin() + Index, Item);
    }

    /** 제거 */
    void RemoveAt(int Index)
    {
        this->erase(this->begin() + Index);
    }

    bool Remove(const T& Item)
    {
        auto it = std::find(this->begin(), this->end(), Item);
        if (it != this->end())
        {
            this->erase(it);
            return true;
        }
        return false;
    }

    int RemoveAll(const T& Item)
    {
        auto oldSize = this->size();
        this->erase(std::remove(this->begin(), this->end(), Item), this->end());
        return static_cast<int>(oldSize - this->size());
    }

    /** 크기 관련 */
    int Num() const
    {
        return static_cast<int>(this->size());
    }

    bool IsEmpty() const
    {
        return this->empty();
    }

    void Empty()
    {
        this->clear();
    }

    // 보유 용량(capacity)을 실제 크기(Num)에 맞춰 축소
    void Shrink()
    {
        this->shrink_to_fit();
    }

    void Reserve(int Capacity)
    {
        this->reserve(static_cast<size_t>(Capacity));
    }

    void SetNum(int NewSize)
    {
        this->resize(static_cast<size_t>(NewSize));
    }

    void SetNum(int NewSize, const T& DefaultValue)
    {
        this->resize(static_cast<size_t>(NewSize), DefaultValue);
    }

    /** 접근 */
    T& Last()
    {
        return this->back();
    }

    const T& Last() const
    {
        return this->back();
    }

    /** Stack 기능 - std::stack 대체 */
    void Push(const T& Item)
    {
        this->push_back(Item);
    }

    T Pop()
    {
        T Item = this->back();
        this->pop_back();
        return Item;
    }

    /** 검색 */
    int Find(const T& Item) const
    {
        auto it = std::find(this->begin(), this->end(), Item);
        return (it != this->end()) ? static_cast<int>(std::distance(this->begin(), it)) : -1;
    }

    bool Contains(const T& Item) const
    {
        return Find(Item) != -1;
    }

    /** 정렬 */
    void Sort()
    {
        std::sort(this->begin(), this->end());
    }

    template<typename Predicate>
    void Sort(Predicate Pred)
    {
        std::sort(this->begin(), this->end(), Pred);
    }
};
template<typename T, typename Alloc = std::allocator<T>>
using TLinkedList = std::list<T, Alloc>;
template<typename T, typename Alloc = std::allocator<T>>
using TDoubleLinkedList = std::list<T, Alloc>;
template<typename T, typename Hash = std::hash<T>, typename Eq = std::equal_to<T>, typename Alloc = std::allocator<T>>
using TSet = std::unordered_set<T, Hash, Eq, Alloc>;
template<typename KeyType, typename ValueType, typename Hasher = std::hash<KeyType>, typename Equal = std::equal_to<KeyType>, typename Alloc = std::allocator<std::pair<const KeyType, ValueType>>>
class TMap : public std::unordered_map<KeyType, ValueType, Hasher, Equal, Alloc>{
public:
	using std::unordered_map<KeyType, ValueType, Hasher, Equal, Alloc>::unordered_map;

    /** 요소 추가/수정 */
    void Add(const KeyType& Key, const ValueType& Value)
    {
        (*this)[Key] = Value;
    }

    template<typename... Args>
    void Emplace(const KeyType& Key, Args&&... args)
    {
        this->emplace(Key, ValueType(std::forward<Args>(args)...));
    }

    /** 제거 */
    bool Remove(const KeyType& Key)
    {
        return this->erase(Key) > 0;
    }

    /** 크기 관련 */
    int Num() const
    {
        return static_cast<int>(this->size());
    }

    bool IsEmpty() const
    {
        return this->empty();
    }

    void Empty()
    {
        this->clear();
    }

    /** 검색 */
    bool Contains(const KeyType& Key) const
    {
        return this->find(Key) != this->end();
    }

    ValueType* Find(const KeyType& Key)
    {
        auto it = this->find(Key);
        return (it != this->end()) ? &it->second : nullptr;
    }

    const ValueType* Find(const KeyType& Key) const
    {
        auto it = this->find(Key);
        return (it != this->end()) ? &it->second : nullptr;
    }

    /** 찾거나 기본값 반환 */
    ValueType FindRef(const KeyType& Key) const
    {
        auto it = this->find(Key);
        return (it != this->end()) ? it->second : ValueType{};
    }

    /** 키/값 배열 반환 */
    TArray<KeyType> GetKeys() const
    {
        TArray<KeyType> Keys;
        Keys.reserve(this->size());
        for (const auto& Pair : *this)
        {
            Keys.Add(Pair.first);
        }
        return Keys;
    }

    TArray<ValueType> GetValues() const
    {
        TArray<ValueType> Values;
        Values.Reserve(this->size());
        for (const auto& Pair : *this)
        {
            Values.Add(Pair.second);
        }
        return Values;
    }
};
template<typename T1, typename T2>
using TPair = std::pair<T1, T2>;
template<typename T, size_t N>
using TStaticArray = std::array<T, N>;
template<typename T, typename Container = std::deque<T>>
using TQueue = std::queue<T, Container>;
template<typename T>
using TDeque = std::deque<T>;
template<typename T>
using TOptional = std::optional<T>;
template<typename T>
using TFunction = std::function<T>;

using FString = std::string;
using uint8 = std::uint8_t;
using int8 = std::int8_t;
using uint16 = std::uint16_t;
using int16 = std::int16_t;
using uint32 = std::uint32_t;
using int32 = std::int32_t;
using uint64 = std::uint64_t;
using int64 = std::int64_t;
