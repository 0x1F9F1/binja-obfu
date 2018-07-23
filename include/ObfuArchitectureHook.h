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

#include <vector>
#include <unordered_map>

#include <mutex>

namespace PatchBuilder
{
    enum class TokenType
    {
        Instruction,
        Operand
    };

    struct Token
    {
        TokenType Type;
        size_t Value;
    };

    struct Patch
    {
        size_t Size;
        std::vector<Token> Tokens;

        bool Evaluate(LowLevelILFunction& il) const;
    };

    struct PatchCollection
    {
        std::unordered_map<uintptr_t, Patch> m_Patches;
        mutable std::mutex m_Mutex;

        void AddPatch(uintptr_t address, Patch patch);
        const Patch* GetPatch(uintptr_t address) const;
    };

    void AddPatch(BinaryView& view, uintptr_t address, Patch patch);
    const Patch* GetPatch(LowLevelILFunction& il, uintptr_t address);
}

class ObfuArchitectureHook
    : public ArchitectureHook
{
public:
    using ArchitectureHook::ArchitectureHook;

    bool GetInstructionLowLevelIL(const uint8_t* data, uint64_t addr, size_t& len, LowLevelILFunction& il) override;
};
