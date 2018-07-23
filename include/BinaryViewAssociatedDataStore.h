#pragma once

#include "BinaryNinja.h"

#include "ObjectDestructionNotification.h"

#include <unordered_map>
#include <memory>

#include <mutex>

template <typename T>
class BinaryViewAssociatedDataStore
    : protected ObjectDestructionNotification
{
protected:
    using SessionDataMap = std::unordered_map<BNBinaryView*, std::unique_ptr<T>>;

    SessionDataMap m_SessionData;
    mutable std::mutex m_Mutex;

    void DestructBinaryView(BNBinaryView* view) override
    {
        std::lock_guard<std::mutex> guard(m_Mutex);

        m_SessionData.erase(view);
    }

public:
    T* Get(BNBinaryView* view)
    {
        std::lock_guard<std::mutex> guard(m_Mutex);

        auto find = m_SessionData.find(view);

        if (find != m_SessionData.end())
        {
            return find->second.get();
        }

        return nullptr;
    }

    const T* Get(BNBinaryView* view) const
    {
        return const_cast<BinaryViewAssociatedDataStore*>(this)->Get(view);
    }

    T* GetOrCreate(BNBinaryView* view)
    {
        T* result = Get(view);

        if (result != nullptr)
        {
            return result;
        }

        std::lock_guard<std::mutex> guard(m_Mutex);
        
        return m_SessionData.emplace(view, std::make_unique<T>()).first->second.get();
    }
};

