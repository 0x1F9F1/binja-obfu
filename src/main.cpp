#include "BinaryNinja.h"

#include "ObfuArchitectureHook.h"

void RegisterObfuHook(const std::string& arch_name)
{
    Architecture* hook = new ObfuArchitectureHook(Architecture::GetByName(arch_name));
    Architecture::Register(hook);
}

extern "C"
{
    BINARYNINJAPLUGIN bool CorePluginInit()
    {
        RegisterObfuHook("x86");
        RegisterObfuHook("x86_64");

        PluginCommand::RegisterForAddress("Add NOP Patch", "Fixes Certain Obfuscation Methods", [](BinaryView* view, uint64_t address)
        {
            AddPatch(*view, address, 0);
        });

        LogInfo("Loaded ObfuArchitectureHook");

        return true;
    }
};