#pragma once

#include "BinaryNinja.h"

#include "ObjectDestructionNotification.h"

#include <unordered_map>
#include <memory>

template <typename T>
class BinaryViewAssociatedDataStore
    : protected ObjectDestructionNotification
{
protected:
    using SessionDataMap = std::unordered_map<BNBinaryView*, std::unique_ptr<T>>;

    SessionDataMap m_SessionData;

    void DestructBinaryView(BNBinaryView* view) override
    {
        m_SessionData.erase(view);
    }

public:
    T* Get(BNBinaryView* view)
    {
        auto find = m_SessionData.find(view);

        if (find != m_SessionData.end())
        {
            return find->second.get();
        }

        return nullptr;
    }

    T* GetOrCreate(BNBinaryView* view)
    {
        T* result = Get(view);

        if (result != nullptr)
        {
            return result;
        }

        auto inserted = m_SessionData.emplace(view, std::make_unique<T>());
        
        return inserted.first->second.get();
    }
};

