#pragma once

#include "BinaryNinja.h"
#include "BinaryViewAssociatedDataStore.h"

using PatchData = int;

void AddPatch(BinaryView& view, uintptr_t address, PatchData patch);

class ObfuArchitectureHook
    : public ArchitectureHook
{
public:
    using ArchitectureHook::ArchitectureHook;

    bool GetInstructionLowLevelIL(const uint8_t* data, uint64_t addr, size_t& len, LowLevelILFunction& il) override;
};