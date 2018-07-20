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
