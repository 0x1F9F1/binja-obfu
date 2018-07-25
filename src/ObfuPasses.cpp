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

#include "fmt/format.h"

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

            if (MLIL_SSA_GetIndirectBranchCondition(*mlil_ssa, last, branch, condition, true_val, false_val))
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
