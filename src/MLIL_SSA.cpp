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

#include "MLIL_SSA.h"

bool MLIL_SSA_TraceVar(
    MediumLevelILFunction& func,
    MediumLevelILInstruction& var)
{
    for (size_t i = 0; i < 100; ++i)
    {
        if (var.operation == MLIL_VAR_SSA)
        {
            size_t idx = func.GetSSAVarDefinition(var.As<MLIL_VAR_SSA>().GetSourceSSAVariable());

            if (idx > func.GetInstructionCount())
            {
                return true;
            }

            var = func.GetInstruction(idx);
        }
        else if (var.operation == MLIL_SET_VAR_SSA)
        {
            var = var.As<MLIL_SET_VAR_SSA>().GetSourceExpr();
        }
        else
        {
            return true;
        }
    }

    return false;
}

std::vector<MediumLevelILInstruction> MLIL_SSA_GetVarDefinitions(
    MediumLevelILFunction& func,
    const MediumLevelILSSAVariableList& vars)
{
    std::vector<MediumLevelILInstruction> results;

    results.reserve(vars.size());

    for (const SSAVariable& var : vars)
    {
        results.emplace_back(func.GetInstruction(func.GetSSAVarDefinition(var)));
    }

    return results;
}

bool MLIL_SSA_SolveBranchDependence(
    MediumLevelILFunction& func,
    MediumLevelILInstruction& lhs,
    MediumLevelILInstruction& rhs,
    MediumLevelILInstruction& out_branch,
    MediumLevelILInstruction& out_true_val,
    MediumLevelILInstruction& out_false_val)
{
    std::unordered_map<size_t, BNILBranchDependence> lhs_branches = lhs.GetAllBranchDependence();
    std::unordered_map<size_t, BNILBranchDependence> rhs_branches = rhs.GetAllBranchDependence();

    for (const auto& lhs_branch : lhs_branches)
    {
        auto rhs_find = rhs_branches.find(lhs_branch.first);

        if (rhs_find == rhs_branches.end())
        {
            continue;
        }

        if (lhs_branch.second == rhs_find->second)
        {
            continue;
        }

        out_branch = func.GetInstruction(lhs_branch.first);

        if (out_branch.operation != MLIL_IF)
        {
            continue;
        }

        if (lhs_branch.second != FalseBranchDependent)
        {
            out_true_val = lhs;
            out_false_val = rhs;
        }
        else
        {
            out_true_val = rhs;
            out_false_val = lhs;
        }

        return true;
    }

    return false;
}

bool MLIL_SSA_GetConditionalMoveSource(
    MediumLevelILFunction& func,
    MediumLevelILInstruction& var,
    MediumLevelILInstruction& out_branch,
    MediumLevelILInstruction& out_condition,
    MediumLevelILInstruction& out_true_val,
    MediumLevelILInstruction& out_false_val)
{
    if (var.operation != MLIL_VAR_PHI)
    {
        return false;
    }

    MediumLevelILSSAVariableList sources = var.As<MLIL_VAR_PHI>().GetSourceSSAVariables();

    if (sources.size() != 2)
    {
        return false;
    }

    std::vector<MediumLevelILInstruction> vars = MLIL_SSA_GetVarDefinitions(func, sources);

    if (vars.size() != 2)
    {
        return false;
    }

    for (MediumLevelILInstruction& var : vars)
    {
        if (var.operation != MLIL_SET_VAR_SSA)
        {
            return false;
        }
    }

    if (!MLIL_SSA_SolveBranchDependence(func, vars[0], vars[1], out_branch, out_true_val, out_false_val))
    {
        return false;
    }

    if (out_branch.operation != MLIL_IF)
    {
        return false;
    }

    out_condition = out_branch.As<MLIL_IF>().GetConditionExpr();

    if (!MLIL_SSA_TraceVar(func, out_condition))
    {
        return false;
    }

    return true;
}

bool MLIL_SSA_GetIndirectBranchCondition(
    MediumLevelILFunction& func,
    MediumLevelILInstruction& branch,
    MediumLevelILInstruction& out_branch,
    MediumLevelILInstruction& out_condition,
    MediumLevelILInstruction& out_true_val,
    MediumLevelILInstruction& out_false_val)
{
    if (branch.operation != MLIL_JUMP_TO)
    {
        return false;
    }

    MediumLevelILInstruction dest = branch.As<MLIL_JUMP_TO>().GetDestExpr();
    PossibleValueSet branchSet = dest.GetPossibleValues();

    if (branchSet.state != InSetOfValues)
    {
        return false;
    }

    if (branchSet.valueSet.size() != 2)
    {
        return false;
    }

    if (!MLIL_SSA_TraceVar(func, dest))
    {
        return false;
    }

    return MLIL_SSA_GetConditionalMoveSource(func, dest, out_branch, out_condition, out_true_val, out_false_val);
}
