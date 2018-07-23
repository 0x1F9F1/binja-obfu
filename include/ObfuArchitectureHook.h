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

        bool Evaluate(LowLevelILFunction& il);
    };

    struct PatchCollection
    {
        std::unordered_map<uintptr_t, Patch> m_Patches;
        std::mutex m_Mutex;

        void AddPatch(uintptr_t address, Patch patch);
        Patch* GetPatch(uintptr_t address);
    };

    void AddPatch(BinaryView& view, uintptr_t address, Patch patch);
    Patch* GetPatch(LowLevelILFunction& il, uintptr_t address);
}

class ObfuArchitectureHook
    : public ArchitectureHook
{
public:
    using ArchitectureHook::ArchitectureHook;

    bool GetInstructionLowLevelIL(const uint8_t* data, uint64_t addr, size_t& len, LowLevelILFunction& il) override;
};