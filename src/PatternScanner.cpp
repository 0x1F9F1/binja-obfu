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

#include "PatternScanner.h"

#include "../vendor/mem/mem.h"
#include "BackgroundTaskThread.h"

#include <thread>
#include <mutex>

#include <chrono>

const size_t MAX_SCAN_RESULTS = 1000;

template <typename ForwardIt, typename UnaryFunction>
void parallel_for_each(ForwardIt first, ForwardIt last, const UnaryFunction& func)
{
#if 1
    uint32_t thread_count = std::thread::hardware_concurrency();

    if (thread_count == 0)
    {
        thread_count = 4;
    }

    std::mutex mutex;

    const auto thread_loop = [&]
    {
        while (true)
        {
            std::unique_lock<std::mutex> guard(mutex);

            if (first != last)
            {
                auto value = *first++;

                guard.unlock();

                func(std::move(value));
            }
            else
            {
                break;
            }
        }
    };

    std::vector<std::thread> threads;

    for (uint32_t i = 0; i < thread_count; ++i)
    {
        threads.emplace_back(thread_loop);
    }

    for (auto& thread : threads)
    {
        thread.join();
    }
#else
    std::for_each(first, last, func);
#endif
}

using stopwatch = std::chrono::steady_clock;

void ScanForArrayOfBytesTask(Ref<BackgroundTask> task, Ref<BinaryView> view, std::string pattern_string)
{
    mem::pattern pattern(pattern_string.c_str());

    std::vector<Segment> segments = view->GetSegments();
    std::vector<uint64_t> results;
    std::mutex mutex;

    const auto start_time = stopwatch::now();

    parallel_for_each(segments.begin(), segments.end(), [&] (const Segment& segment)
    {
        if (task->IsCancelled())
        {
            return;
        }

        DataBuffer data = view->ReadBuffer(segment.start, segment.length);

        std::vector<mem::pointer> scan_results = pattern.scan_all({ data.GetData(), data.GetLength() });

        if (task->IsCancelled())
        {
            return;
        }

        std::unique_lock<std::mutex> lock(mutex);

        for (mem::pointer result : scan_results)
        {
            results.push_back(result.shift(data.GetData(), segment.start).as<uint64_t>());
        }

        task->SetProgressText(fmt::format("Scanning for pattern: \"{}\", found {} results", pattern_string, results.size()));
    });

    const auto end_time = stopwatch::now();

    if (task->IsCancelled())
    {
        return;
    }

    std::string report;
        
    report += fmt::format("Found {} results for \"{}\" in {} ms:\n", results.size(), pattern_string, std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count());

    const auto plength = pattern.size();

    if (plength > 0)
    {
        const auto& pbytes = pattern.bytes();
        const auto& pmasks = pattern.masks();

        report += fmt::format("Pattern: Length {}, \"{}\", \"{}\"\n",
            plength,
            mem::region(pbytes.data(), pbytes.size()).hex(true, true),
            mem::region(pmasks.data(), pmasks.size()).hex(true, true)
        );
    }

    if (results.size() > MAX_SCAN_RESULTS)
    {
        report += fmt::format("Too many results, only showing first {}.\n", MAX_SCAN_RESULTS);

        results.resize(MAX_SCAN_RESULTS);
    }

    report += "\n";

    for (uint64_t result : results)
    {
        report += fmt::format("0x{:X}", result);

        auto blocks = view->GetBasicBlocksForAddress(result);

        if (!blocks.empty())
        {
            report += " (in ";

            for (size_t i = 0; i < blocks.size(); ++i)
            {
                if (i)
                {
                    report += ", ";
                }

                report += blocks[i]->GetFunction()->GetSymbol()->GetFullName();
            }

            report += ")";
        }

        report += "\n";
    }

    BinaryNinja::ShowPlainTextReport("Scan Results", report);
}

void ScanForArrayOfBytes(BinaryView* view)
{
    std::string pattern_string;

    if (BinaryNinja::GetTextLineInput(pattern_string, "Pattern", "Input Pattern"))
    {
        Ref<BackgroundTaskThread> task = new BackgroundTaskThread(fmt::format("Scanning for pattern: \"{}\"", pattern_string));

        task->Run(&ScanForArrayOfBytesTask, Ref<BinaryView>(view), pattern_string);
    }
}