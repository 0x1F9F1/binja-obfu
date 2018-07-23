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

class ObjectDestructionNotification
{
protected:
    BNObjectDestructionCallbacks m_callbacks;

    static void DestructBinaryViewCallback(void* ctxt, BNBinaryView* view);
    static void DestructFileMetadataCallback(void* ctxt, BNFileMetadata* file);
    static void DestructFunctionCallback(void* ctxt, BNFunction* func);

public:
    ObjectDestructionNotification();

    virtual ~ObjectDestructionNotification();

    virtual void DestructBinaryView(BNBinaryView* view);
    virtual void DestructFileMetadata(BNFileMetadata* file);
    virtual void DestructFunction(BNFunction* func);
};
