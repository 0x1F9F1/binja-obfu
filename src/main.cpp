#include "BinaryNinja.h"

#include "ObfuArchitectureHook.h"

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

        LogInfo("Loaded ObfuArchitectureHook");

        return true;
    }
};