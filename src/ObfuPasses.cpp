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

void LabelIndirectBranches(
    BinaryView* view,
    Function* func)
{
    Ref<MediumLevelILFunction> mlil = func->GetMediumLevelIL()->GetSSAForm();

    MediumLevelILInstruction branch;
    MediumLevelILInstruction condition;
    MediumLevelILInstruction true_val;
    MediumLevelILInstruction false_val;

    for (auto block : mlil->GetBasicBlocks())
    {
        if (block->GetLength() > 1)
        {
            auto last = mlil->GetInstruction(block->GetEnd() - 1);

            if (MLIL_SSA_GetIndirectBranchCondition(*mlil, last, branch, condition, true_val, false_val))
            {
                branch = branch.GetNonSSAForm();
                condition = condition.GetNonSSAForm();
                true_val = true_val.GetNonSSAForm();
                false_val = false_val.GetNonSSAForm();

                char buffer[256];

                snprintf(buffer, 256, "%zu @ 0x%zX", condition.instructionIndex, condition.address);

                func->SetCommentForAddress(last.address, buffer);
            }
        }
    }
}
