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

#include <thread>
#include <mutex>

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

void ScanForArrayOfBytes(BinaryView* view)
{
    std::string pattern_string;

    if (BinaryNinja::GetTextLineInput(pattern_string, "Pattern", "Input Pattern"))
    {
        BinjaLog(InfoLog, "Scanning for {}", pattern_string);

        mem::pattern pattern(mem::ida_style, pattern_string.c_str());

        std::vector<Segment> segments = view->GetSegments();
        std::vector<uint64_t> results;
        std::mutex mutex;

        const auto start_time = stopwatch::now();

        parallel_for_each(segments.begin(), segments.end(), [&] (const Segment& segment)
        {
            DataBuffer data = view->ReadBuffer(segment.start, segment.length);

            std::vector<mem::pointer> scan_results = pattern.scan_all({ data.GetData(), data.GetLength() });

            std::unique_lock<std::mutex> lock(mutex);

            for (mem::pointer result : scan_results)
            {
                results.push_back(result.shift(data.GetData(), segment.start).as<uint64_t>());
            }
        });

        const auto end_time = stopwatch::now();

        std::string report;
        
        report += fmt::format("Found {} result/s for \"{}\" in {} ms:\n\n", results.size(), pattern_string, std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count());

        if (results.size() > MAX_SCAN_RESULTS)
        {
            report += fmt::format("Too many results, only showing first {}.\n\n", MAX_SCAN_RESULTS);

            results.resize(MAX_SCAN_RESULTS);
        }

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

        BinaryNinja::ShowPlainTextReport("Results", report);
    }
}