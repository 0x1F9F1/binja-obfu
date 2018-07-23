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

#include "ObjectDestructionNotification.h"

void ObjectDestructionNotification::DestructBinaryViewCallback(void* ctxt, BNBinaryView* view)
{
    static_cast<ObjectDestructionNotification*>(ctxt)->DestructBinaryView(view);
}

void ObjectDestructionNotification::DestructFileMetadataCallback(void* ctxt, BNFileMetadata* file)
{
    static_cast<ObjectDestructionNotification*>(ctxt)->DestructFileMetadata(file);
}

void ObjectDestructionNotification::DestructFunctionCallback(void* ctxt, BNFunction* func)
{
    static_cast<ObjectDestructionNotification*>(ctxt)->DestructFunction(func);
}

ObjectDestructionNotification::ObjectDestructionNotification()
{
    m_callbacks.context = this;

    BNRegisterObjectDestructionCallbacks(&m_callbacks);
}

ObjectDestructionNotification::~ObjectDestructionNotification()
{
    BNUnregisterObjectDestructionCallbacks(&m_callbacks);
}

void ObjectDestructionNotification::DestructBinaryView(BNBinaryView* view)
{
    (void)view;
}

void ObjectDestructionNotification::DestructFileMetadata(BNFileMetadata* file)
{
    (void)file;
}

void ObjectDestructionNotification::DestructFunction(BNFunction* func)
{
    (void)func;
}
