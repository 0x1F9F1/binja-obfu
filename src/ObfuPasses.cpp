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
        if (block->GetLength() > 1)
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
        if (block->GetLength() > 1)
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
}

void FixObfuscation(
    BackgroundTask* task,
    BinaryView* view,
    Function* func)
{
    for (size_t i = 0; i < 100; ++i)
    {
        if (task)
        {
            task->SetProgressText(fmt::format("Deobfuscating {0}, Pass {1}", func->GetSymbol()->GetShortName(), i));
        }

        if (FixTails(view, func))
        {
            func->Reanalyze();

            view->UpdateAnalysisAndWait();
        }
        else
        {
            break;
        }
    }

    if (task)
    {
        task->SetProgressText(fmt::format("Deobfuscating {0}, Post-Analysis", func->GetSymbol()->GetShortName()));
    }

    LabelIndirectBranches(view, func);

    PatchBuilder::SavePatches(*view);
}