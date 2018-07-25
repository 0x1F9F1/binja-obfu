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

#include "nlohmann/json.hpp"
using json = nlohmann::json;

#include <vector>
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

    void AddPatch(BinaryView& view, uintptr_t address, Patch patch);
    const Patch* GetPatch(LowLevelILFunction& il, uintptr_t address);

    void SavePatches(BinaryView& view);

    void to_json(json& j, const Token& p);
    void from_json(const json& j, Token& p);

    void to_json(json& j, const Patch& p);
    void from_json(const json& j, Patch& p);
}
