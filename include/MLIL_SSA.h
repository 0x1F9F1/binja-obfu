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

#pragma once

#include "BinaryNinja.h"

bool MLIL_SSA_TraceVar(
    MediumLevelILInstruction& var);

std::vector<MediumLevelILInstruction> MLIL_SSA_GetVarDefinitions(
    MediumLevelILFunction& func,
    const MediumLevelILSSAVariableList& vars);

bool MLIL_SSA_SolveBranchDependence(
    MediumLevelILInstruction& lhs,
    MediumLevelILInstruction& rhs,
    MediumLevelILInstruction& out_branch,
    MediumLevelILInstruction& out_true_val,
    MediumLevelILInstruction& out_false_val);

bool MLIL_SSA_GetConditionalMoveSource(
    MediumLevelILInstruction& var,
    MediumLevelILInstruction& out_branch,
    MediumLevelILInstruction& out_condition,
    MediumLevelILInstruction& out_true_val,
    MediumLevelILInstruction& out_false_val);

bool MLIL_SSA_GetIndirectBranchCondition(
    MediumLevelILInstruction& branch,
    MediumLevelILInstruction& out_branch,
    MediumLevelILInstruction& out_condition,
    MediumLevelILInstruction& out_true_val,
    MediumLevelILInstruction& out_false_val);
