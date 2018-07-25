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

#include <thread>

class BackgroundTaskThread
    : public BackgroundTask
{
protected:
    std::thread thread_;

public:
    BackgroundTaskThread(const std::string& initialText)
        : BackgroundTask(initialText, true)
    { }

    template <typename Func, typename... Args>
    void Run(Func&& func, Args&&... args)
    {
        thread_ = std::thread([ ] (Ref<BackgroundTaskThread> task, auto func, auto... args) -> void
        {
            std::invoke(std::move(func), task.GetPtr(), std::move(args)...);

            BinjaLog(InfoLog, "Done");

            task->Finish();
            task->thread_.detach();

            task = nullptr;
        }, Ref<BackgroundTaskThread>(this), std::forward<Func>(func), std::forward<Args>(args)...);
    }

    void Join()
    {
        if (thread_.joinable())
        {
            thread_.join();
        }
    }
};
