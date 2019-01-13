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

#include "BinaryNinja.h"

#include "ObfuArchitectureHook.h"
#include "PatchBuilder.h"
#include "ObfuPasses.h"
#include "BackgroundTaskThread.h"

void RegisterObfuHook(const std::string& arch_name)
{
    Ref<Architecture> hook = new ObfuArchitectureHook(Architecture::GetByName(arch_name));

    Architecture::Register(hook);
}

void FixObfuscationBackgroundTask(Ref<BinaryView> view, Ref<Function> func)
{
    Ref<BackgroundTaskThread> task = new BackgroundTaskThread("De-Obfuscating");

    task->Run(&FixObfuscation, Ref<BinaryView>(view), Ref<Function>(func), true);
}

void FixObfuscationTask(Ref<BinaryView> view, Ref<Function> func)
{
    FixObfuscation(nullptr, view, func, false);
}

void LoadPatchesTask(BinaryView* view)
{
    PatchBuilder::LoadPatches(*view);
}

void SavePatchesTask(BinaryView* view)
{
    PatchBuilder::SavePatches(*view);
}

extern "C"
{
    BINARYNINJAPLUGIN bool CorePluginInit()
    {
        for (const char* arch : { "x86", "x86_64" })
        {
            RegisterObfuHook(arch);
        }

        PluginCommand::RegisterForFunction("Obfuscation\\Fix Obfuscation Background", "", &FixObfuscationBackgroundTask);
        PluginCommand::RegisterForFunction("Obfuscation\\Fix Obfuscation", "", &FixObfuscationTask);
        PluginCommand::Register("Obfuscation\\Load Patches", "", &LoadPatchesTask);
        PluginCommand::Register("Obfuscation\\Save Patches", "", &SavePatchesTask);

        BinjaLog(InfoLog, "Loaded binja-obfu");

        return true;
    }
};
