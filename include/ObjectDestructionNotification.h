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
