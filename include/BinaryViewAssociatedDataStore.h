// Copyright (C) 2018 Brick
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

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

    void Set(BNBinaryView* view, std::unique_ptr<T> value)
    {
        std::lock_guard<std::mutex> guard(m_Mutex);

        m_SessionData.emplace(view, std::move(value));
    }

    const T* Get(BNBinaryView* view) const
    {
        return const_cast<BinaryViewAssociatedDataStore*>(this)->Get(view);
    }
};
