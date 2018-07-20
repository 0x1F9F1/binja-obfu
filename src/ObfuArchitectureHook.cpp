#include "ObfuArchitectureHook.h"

#include <unordered_set>

BinaryViewAssociatedDataStore<std::unordered_map<uintptr_t, PatchData>> g_PatchDataStore;

PatchData* GetPatch(LowLevelILFunction& il, uintptr_t address)
{
    // auto function = il.GetFunction();

    auto function = BNGetLowLevelILOwnerFunction(il.m_object);

    if (function == nullptr)
    {
        return nullptr;
    }

    auto view = BNGetFunctionData(function);

    if (view == nullptr)
    {
        return nullptr;
    }

    auto view_data = g_PatchDataStore.Get(view);

    if (view_data == nullptr)
    {
        return nullptr;
    }

    auto patch_data = view_data->find(address);
    
    if (patch_data == view_data->end())
    {
        return nullptr;
    }

    return &patch_data->second;
}

void AddPatch(BinaryView& view, uintptr_t address, PatchData patch)
{
    g_PatchDataStore.GetOrCreate(view.m_object)->emplace(address, std::move(patch));
}

bool ObfuArchitectureHook::GetInstructionLowLevelIL(const uint8_t* data, uint64_t addr, size_t& len, LowLevelILFunction& il)
{
    auto patch_data = GetPatch(il, addr);

    if (patch_data)
    {
        il.AddInstruction(il.Nop());

        InstructionInfo info;

        GetInstructionInfo(data, addr, len, info);

        len = info.length;

        return true;
    }

    return ArchitectureHook::GetInstructionLowLevelIL(data, addr, len, il);
}

