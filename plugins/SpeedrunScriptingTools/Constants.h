#pragma once

#include <Action.h>
#include <Condition.h>

#include <GWCA/GameContainers/GamePos.h>

#include <cassert>
#include <variant>
#include <string>

template <typename T>
struct NamedConstant 
{
    uint32_t id; // For storage and serialization purposes only, not shown to the user.

    std::string name;
    T value;
};

class ConstantsManager {
public:
    using Position = GW::Vec2f;
    using Polygon = std::vector<GW::Vec2f>;
    using ActionSequence = std::vector<std::shared_ptr<Action>>;
    using ConditionSet = std::vector<std::shared_ptr<Condition>>;

    static ConstantsManager& getInstance()
    {
        static ConstantsManager manager;
        return manager;
    }

    template <typename T>
    std::string_view getValue(uint32_t id)
    {
        return std::ranges::find_if(getStorage<T>(), [&](const auto& c){return c.id == id;})->name;
    }

    template <typename T>
    const T& getValue(uint32_t id)
    {
        return std::ranges::find_if(getStorage<T>(), [&](const auto& c){return c.id == id;})->value;
    }

    template <typename T>
    void addNew()
    {
        for (uint32_t i = 0;; ++i) 
        {
            auto& storage = getStorage<T>();
            const auto it = std::ranges::find_if(storage, [&](const auto& c){return c.id == i;});
            if (it == storage.end()) 
            {
                storage.push_back({i, "", T{}});
                return;
            }
        }
    }

    template <typename T>
    void insert(NamedConstant<T>&& constant)
    {
        auto& storage = getStorage<T>();
        storage.push_back(std::forward<NamedConstant<T>>(constant));
    }

    template<typename T>
    std::vector<NamedConstant<T>>& list()
    {
        return getStorage<T>();
    }

    ConstantsManager(const ConstantsManager&) = delete;
    ConstantsManager(ConstantsManager&&) = delete;

private:
    ConstantsManager() = default;

    template <typename T>
    std::vector<NamedConstant<T>>& getStorage()
    {
        if constexpr (std::is_same_v<T, int>) return ints;
        if constexpr (std::is_same_v<T, float>) return floats;
        if constexpr (std::is_same_v<T, Position>) return positions;
        if constexpr (std::is_same_v<T, Polygon>) return polygons;
        if constexpr (std::is_same_v<T, ActionSequence>) return actions;
        if constexpr (std::is_same_v<T, ConditionSet>) return conditions;
        assert(false);
    }
    std::vector<NamedConstant<int>> ints;
    std::vector<NamedConstant<float>> floats;
    std::vector<NamedConstant<Position>> positions;
    std::vector<NamedConstant<Polygon>> polygons;
    std::vector<NamedConstant<ActionSequence>> actions;
    std::vector<NamedConstant<ConditionSet>> conditions;
};

template <typename T>
class VoC // Value or NamedConstant id
{
public:
    const T& get() 
    { 
        if (std::holds_alternative<T>(content)) 
            return std::get<T>(content);
        return ConstantsManager::getInstance().load(std::get<uint32_t>(content));
    }
    
    void set(uint32_t id)
    {
        content = id;
    }

    void set(T val) 
    { 
        content = val; 
    }



private:
    std::variant<T, uint32_t> content;
};
