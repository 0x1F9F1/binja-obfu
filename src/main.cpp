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

void RegisterObfuHook(const std::string& arch_name)
{
    Architecture* hook = new ObfuArchitectureHook(Architecture::GetByName(arch_name));
    Architecture::Register(hook);
}

void ProcessPatch(BinaryView* view, const LowLevelILInstruction& insn)
{
    // ecx = ecx + 0xDEADBEEF
    // edx = edx + 0xCAFEF00D

    PatchBuilder::AddPatch(*view, insn.address, {
        view->GetInstructionLength(insn.function->GetArchitecture(), insn.address), {
            // ecx = ecx + 0xDEADBEEF
                { PatchBuilder::TokenType::Operand, view->GetDefaultArchitecture()->GetRegisterByName("ecx") },
                            { PatchBuilder::TokenType::Operand, view->GetDefaultArchitecture()->GetRegisterByName("ecx") },
                        { PatchBuilder::TokenType::Operand, 1 }, // Operand Count
                        { PatchBuilder::TokenType::Operand, 0 }, // Flags
                        { PatchBuilder::TokenType::Operand, 4 }, // Operand Size
                        { PatchBuilder::TokenType::Instruction, BNLowLevelILOperation::LLIL_REG },
                    { PatchBuilder::TokenType::Operand, 0xDEADBEEF },
                    { PatchBuilder::TokenType::Operand, 1 }, // Operand Count
                    { PatchBuilder::TokenType::Operand, 0 }, // Flags
                    { PatchBuilder::TokenType::Operand, 4 }, // Operand Size
                    { PatchBuilder::TokenType::Instruction, BNLowLevelILOperation::LLIL_CONST },
                { PatchBuilder::TokenType::Operand, 2 }, // Operand Count
                { PatchBuilder::TokenType::Operand, 0 }, // Flags
                { PatchBuilder::TokenType::Operand, 4 }, // Operand Size
                { PatchBuilder::TokenType::Instruction, BNLowLevelILOperation::LLIL_ADD },
            { PatchBuilder::TokenType::Operand, 2 }, // Operand Count
            { PatchBuilder::TokenType::Operand, 0 }, // Flags
            { PatchBuilder::TokenType::Operand, 4 }, // Operand Size
            { PatchBuilder::TokenType::Instruction, BNLowLevelILOperation::LLIL_SET_REG },

            // edx = edx + 0xCAFEF00D
                { PatchBuilder::TokenType::Operand, view->GetDefaultArchitecture()->GetRegisterByName("edx") },
                            { PatchBuilder::TokenType::Operand, view->GetDefaultArchitecture()->GetRegisterByName("edx") },
                        { PatchBuilder::TokenType::Operand, 1 }, // Operand Count
                        { PatchBuilder::TokenType::Operand, 0 }, // Flags
                        { PatchBuilder::TokenType::Operand, 4 }, // Operand Size
                        { PatchBuilder::TokenType::Instruction, BNLowLevelILOperation::LLIL_REG },
                    { PatchBuilder::TokenType::Operand, 0xCAFEF00D },
                    { PatchBuilder::TokenType::Operand, 1 }, // Operand Count
                    { PatchBuilder::TokenType::Operand, 0 }, // Flags
                    { PatchBuilder::TokenType::Operand, 4 }, // Operand Size
                    { PatchBuilder::TokenType::Instruction, BNLowLevelILOperation::LLIL_CONST },
                { PatchBuilder::TokenType::Operand, 2 }, // Operand Count
                { PatchBuilder::TokenType::Operand, 0 }, // Flags
                { PatchBuilder::TokenType::Operand, 4 }, // Operand Size
                { PatchBuilder::TokenType::Instruction, BNLowLevelILOperation::LLIL_ADD },
            { PatchBuilder::TokenType::Operand, 2 }, // Operand Count
            { PatchBuilder::TokenType::Operand, 0 }, // Flags
            { PatchBuilder::TokenType::Operand, 4 }, // Operand Size
            { PatchBuilder::TokenType::Instruction, BNLowLevelILOperation::LLIL_SET_REG },
        }
    });
}

extern "C"
{
    BINARYNINJAPLUGIN bool CorePluginInit()
    {
        RegisterObfuHook("x86");
        RegisterObfuHook("x86_64");

        PluginCommand::RegisterForLowLevelILInstruction("Add Test Patch", ":thinking:", &ProcessPatch);
        PluginCommand::RegisterForFunction("Label Indirect Branches", ":thonking:", &LabelIndirectBranches);

        LogInfo("Loaded ObfuArchitectureHook");

        return true;
    }
};
