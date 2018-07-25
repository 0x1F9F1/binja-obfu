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

#include "ObfuPasses.h"
#include "MLIL_SSA.h"
#include "MLIL.h"
#include "PatchBuilder.h"

#include "fmt/format.h"

bool CheckTailXrefs(BinaryView* view, Function* func, Function* tail)
{
    std::set<Ref<Function>> funcs;

    for (const ReferenceSource& source : view->GetCodeReferences(tail->GetStart()))
    {
        if (source.func)
        {
            funcs.emplace(source.func);
        }
    }

    if (funcs.find(func) != funcs.end())
    {
        funcs.erase(func);
    }

    return funcs.size() == 0;
}

size_t FixTails(BinaryView* view, Function* func)
{
    size_t total = 0;

    Ref<LowLevelILFunction> llil = func->GetLowLevelIL();

    for (Ref<BasicBlock> block : llil->GetBasicBlocks())
    {
        LowLevelILInstruction last = llil->GetInstruction(block->GetEnd() - 1);

        Ref<Function> tail;

        if (last.operation == LLIL_TAILCALL || last.operation == LLIL_JUMP)
        {
            LowLevelILInstruction dest = last.GetDestExpr();

            PossibleValueSet branchSet = dest.GetPossibleValues();

            if (branchSet.state == ConstantValue || branchSet.state == ConstantPointerValue)
            {
                tail = view->GetAnalysisFunction(func->GetPlatform(), branchSet.value);
            }
        }
        else
        {
            continue;
        }

        if (!tail)
        {
            continue;
        }

        if (tail.GetPtr() == func)
        {
            continue;
        }

        if (!tail->WasAutomaticallyDiscovered())
        {
            continue;
        }

        if (!CheckTailXrefs(view, func, tail))
        {
            continue;
        }

        view->RemoveUserFunction(tail);

        ++total;
    }

    return total;
}

void Flatten(std::vector<PatchBuilder::Token>& patches, LowLevelILInstruction& insn)
{
    const std::vector<LowLevelILOperandUsage>& operand_uses = LowLevelILInstruction::operationOperandUsage.at(insn.operation);

    size_t operand_count = operand_uses.size();

    for (size_t i = 0; i < operand_count; ++i)
    {
        LowLevelILOperandUsage operand_usage = operand_uses[i];
        LowLevelILOperandType operand_type = LowLevelILInstruction::operandTypeForUsage.at(operand_usage);

        if (operand_type == ExprLowLevelOperand)
        {
            Flatten(patches, insn.GetRawOperandAsExpr(i));
        }
        else
        {
            patches.push_back({ PatchBuilder::TokenType::Operand, insn.operands[i] });
        }
    }

    patches.insert(patches.end(), std::initializer_list<PatchBuilder::Token> {
        { PatchBuilder::TokenType::Operand, operand_count },
        { PatchBuilder::TokenType::Operand, insn.flags },
        { PatchBuilder::TokenType::Operand, insn.size },
        { PatchBuilder::TokenType::Instruction, static_cast<size_t>(insn.operation) }
    });
}

bool AreValuesExecutable(BinaryView& view, const PossibleValueSet& values)
{
    if (values.state == ImportedAddressValue)
    {
        return true;
    }
    else if (values.state == ConstantValue || values.state == ConstantPointerValue)
    {
        return view.IsOffsetExecutable(values.value);
    }
    else if (values.state == LookupTableValue)
    {
        for (const LookupTableEntry& entry : values.table)
        {
            if (!view.IsOffsetExecutable(entry.toValue))
            {
                return false;
            }
        }

        return true;
    }
    else if (values.state == InSetOfValues)
    {
        for (int64_t value : values.valueSet)
        {
            if (!view.IsOffsetExecutable(value))
            {
                return false;
            }
        }

        return true;
    }

    return false;
}

size_t FixJumps(BinaryView* view, Function* func)
{
    size_t total = 0;

    Ref<LowLevelILFunction> llil = func->GetLowLevelIL();

    const uint32_t stack_register = llil->GetArchitecture()->GetStackPointerRegister();
    const size_t address_size = view->GetAddressSize();

    for (Ref<BasicBlock> block : llil->GetBasicBlocks())
    {
        LowLevelILInstruction last = llil->GetInstruction(block->GetEnd() - 1);

        if ((last.operation == LLIL_RET) ||
            (last.operation == LLIL_JUMP) ||
            (last.operation == LLIL_JUMP_TO) ||
            (last.operation == LLIL_TAILCALL))
        {
            if (PatchBuilder::GetPatch(*llil, last.address))
            {
                continue;
            }

            RegisterValue stack_register_value = last.GetRegisterValue(stack_register);

            if (stack_register_value.state != StackFrameOffset)
            {
                continue;
            }

            int64_t stack_offset = stack_register_value.value;

            std::vector<PatchBuilder::Token> patches;

            LowLevelILInstruction dest = last.GetDestExpr();

            if (dest.operation == LLIL_LOAD)
            {
                LowLevelILInstruction load_source = dest.As<LLIL_LOAD>().GetSourceExpr();

                if (load_source.operation == LLIL_ADD)
                {
                    LowLevelILInstruction add_lhs = load_source.As<LLIL_ADD>().GetLeftExpr();
                    LowLevelILInstruction add_rhs = load_source.As<LLIL_ADD>().GetRightExpr();

                    if ((add_lhs.operation == LLIL_REG) &&
                        (add_lhs.As<LLIL_REG>().GetSourceRegister() == stack_register))
                    {
                        if (add_rhs.operation == LLIL_CONST)
                        {
                            int64_t stack_adjustment = add_rhs.As<LLIL_CONST>().GetConstant();

                            stack_offset += stack_adjustment;

                            // reg = reg + adjustment
                            patches.insert(patches.end(), std::initializer_list<PatchBuilder::Token> {
                                    { PatchBuilder::TokenType::Operand, stack_register },
                                                { PatchBuilder::TokenType::Operand, stack_register },
                                            { PatchBuilder::TokenType::Operand, 1 }, // Operand Count
                                            { PatchBuilder::TokenType::Operand, 0 }, // Flags
                                            { PatchBuilder::TokenType::Operand, address_size }, // Operand Size
                                            { PatchBuilder::TokenType::Instruction, BNLowLevelILOperation::LLIL_REG },
                                        { PatchBuilder::TokenType::Operand, static_cast<size_t>(stack_adjustment) },
                                        { PatchBuilder::TokenType::Operand, 1 }, // Operand Count
                                        { PatchBuilder::TokenType::Operand, 0 }, // Flags
                                        { PatchBuilder::TokenType::Operand, address_size }, // Operand Size
                                        { PatchBuilder::TokenType::Instruction, BNLowLevelILOperation::LLIL_CONST },
                                    { PatchBuilder::TokenType::Operand, 2 }, // Operand Count
                                    { PatchBuilder::TokenType::Operand, 0 }, // Flags
                                    { PatchBuilder::TokenType::Operand, address_size }, // Operand Size
                                    { PatchBuilder::TokenType::Instruction, BNLowLevelILOperation::LLIL_ADD },
                                { PatchBuilder::TokenType::Operand, 2 }, // Operand Count
                                { PatchBuilder::TokenType::Operand, 0 }, // Flags
                                { PatchBuilder::TokenType::Operand, address_size }, // Operand Size
                                { PatchBuilder::TokenType::Instruction, BNLowLevelILOperation::LLIL_SET_REG },
                            });
                        }
                    }
                }
            }

            size_t good_pops = 0;

            for (; good_pops < 16; ++good_pops)
            {
                int64_t current_offset = stack_offset + (good_pops * address_size);

                PossibleValueSet stack_contents = last.GetPossibleStackContents(static_cast<int32_t>(current_offset), address_size);

                if (!AreValuesExecutable(*view, stack_contents))
                {
                    break;
                }
            }

            if (!good_pops)
            {
                continue;
            }

            if (dest.operation == LLIL_REG || dest.operation == LLIL_CONST_PTR)
            {
                Flatten(patches, dest);

                patches.insert(patches.end(), std::initializer_list<PatchBuilder::Token> {
                    { PatchBuilder::TokenType::Operand, 1 }, // Operand Count
                    { PatchBuilder::TokenType::Operand, 0 }, // Flags
                    { PatchBuilder::TokenType::Operand, address_size }, // Operand Size
                    { PatchBuilder::TokenType::Instruction, BNLowLevelILOperation::LLIL_CALL },
                });
            }

            for (size_t i = good_pops; i--;)
            {
                patches.insert(patches.end(), std::initializer_list<PatchBuilder::Token> {
                        { PatchBuilder::TokenType::Operand, LLIL_TEMP(i) },
                        { PatchBuilder::TokenType::Operand, 0 }, // Operand Count
                        { PatchBuilder::TokenType::Operand, 0 }, // Flags
                        { PatchBuilder::TokenType::Operand, address_size }, // Operand Size
                        { PatchBuilder::TokenType::Instruction, BNLowLevelILOperation::LLIL_POP },
                    { PatchBuilder::TokenType::Operand, 2 }, // Operand Count
                    { PatchBuilder::TokenType::Operand, 0 }, // Flags
                    { PatchBuilder::TokenType::Operand, address_size }, // Operand Size
                    { PatchBuilder::TokenType::Instruction, BNLowLevelILOperation::LLIL_SET_REG },

                            { PatchBuilder::TokenType::Operand, LLIL_TEMP(i) },
                        { PatchBuilder::TokenType::Operand, 1 }, // Operand Count
                        { PatchBuilder::TokenType::Operand, 0 }, // Flags
                        { PatchBuilder::TokenType::Operand, address_size }, // Operand Size
                        { PatchBuilder::TokenType::Instruction, BNLowLevelILOperation::LLIL_REG },
                    { PatchBuilder::TokenType::Operand, 1 }, // Operand Count
                    { PatchBuilder::TokenType::Operand, 0 }, // Flags
                    { PatchBuilder::TokenType::Operand, address_size }, // Operand Size
                    { PatchBuilder::TokenType::Instruction, static_cast<size_t>(i ? BNLowLevelILOperation::LLIL_CALL : BNLowLevelILOperation::LLIL_JUMP) },
                });
            }

            if (!patches.empty())
            {
                PatchBuilder::AddPatch(*view, last.address, PatchBuilder::Patch {
                    view->GetInstructionLength(last.function->GetArchitecture(), last.address), patches
                });

                total += 1;
            }
        }
    }

    return total;
}

void LabelIndirectBranches(
    BinaryView* view,
    Function* func)
{
    Ref<MediumLevelILFunction> mlil_ssa = func->GetMediumLevelIL()->GetSSAForm();
    Ref<Architecture> arch = func->GetArchitecture();

    MediumLevelILInstruction branch;
    MediumLevelILInstruction condition;
    MediumLevelILInstruction true_val;
    MediumLevelILInstruction false_val;

    for (Ref<BasicBlock>& block : mlil_ssa->GetBasicBlocks())
    {
        MediumLevelILInstruction last = mlil_ssa->GetInstruction(block->GetEnd() - 1);

        if (MLIL_SSA_GetIndirectBranchCondition(last, branch, condition, true_val, false_val))
        {
            branch = branch.GetNonSSAForm();
            condition = condition.GetNonSSAForm();
            true_val = true_val.GetNonSSAForm();
            false_val = false_val.GetNonSSAForm();

            func->SetCommentForAddress(last.address, fmt::format("{0} @ {1:x}  if ({2}) then {3} else {4}",
                condition.instructionIndex,
                condition.address,
                MLIL_ToString(condition),
                MLIL_ToString(true_val),
                MLIL_ToString(false_val)));

            func->SetUserInstructionHighlight(arch, last.address, BlueHighlightColor);
            func->SetUserInstructionHighlight(arch, condition.address, OrangeHighlightColor);

            if (true_val.operation == MLIL_CONST)
            {
                func->SetUserInstructionHighlight(arch, true_val.As<MLIL_CONST>().GetConstant(), GreenHighlightColor);
            }

            if (false_val.operation == MLIL_CONST)
            {
                func->SetUserInstructionHighlight(arch, false_val.As<MLIL_CONST>().GetConstant(), RedHighlightColor);
            }
        }
    }
}

void LabelPossibleTails(BinaryView* view, Function* func)
{
    Ref<Architecture> arch = func->GetArchitecture();

    Ref<LowLevelILFunction> llil = func->GetLowLevelIL();

    const uint32_t stack_register = arch->GetStackPointerRegister();
    const size_t address_size = view->GetAddressSize();

    for (Ref<BasicBlock> block : llil->GetBasicBlocks())
    {
        LowLevelILInstruction last = llil->GetInstruction(block->GetEnd() - 1);

        RegisterValue stack_register_value = last.GetRegisterValue(stack_register);

        if (stack_register_value.state != StackFrameOffset)
        {
            continue;
        }

        if (stack_register_value.value == 0)
        {
            if (func->GetInstructionHighlight(arch, last.address).alpha == 0)
            {
                func->SetUserInstructionHighlight(arch, last.address, MagentaHighlightColor);
            }
        }
    }
}


void FixObfuscation(
    BackgroundTask* task,
    BinaryView* view,
    Function* func)
{
    size_t passes = 1;

    std::string func_name = func->GetSymbol()->GetShortName();

    for (; passes < 100; ++passes)
    {
        if (task)
        {
            task->SetProgressText(fmt::format("Deobfuscating {0}, Pass {1} Pending", func_name, passes));
        }

        func->Reanalyze();
        view->UpdateAnalysis();

        while (func->NeedsUpdate())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        if (task)
        {
            task->SetProgressText(fmt::format("Deobfuscating {0}, Pass {1} Analyzing", func_name, passes));
        }

        if (FixTails(view, func) ||
            FixJumps(view, func))
        {
            // Beep Boop
        }
        else
        {
            break;
        }
    }

    if (task)
    {
        task->SetProgressText(fmt::format("Deobfuscating {0}, Post-Analysis", func_name));
    }

    LabelIndirectBranches(view, func);
    LabelPossibleTails(view, func);

    PatchBuilder::SavePatches(*view);

    BinjaLog(InfoLog, "Deobfuscated {0} after {1} passes", func_name, passes);
}