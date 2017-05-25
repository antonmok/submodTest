#include "RetainMemory.h"

RetainMemory* RetainMemory::instance_ = NULL;

RetainMemory::RetainMemory()
{
    //ctor
}

RetainMemory::~RetainMemory()
{
    //dtor
}

RetainMemory* RetainMemory::Instance()
{
    if (NULL == instance_)
        instance_ = new RetainMemory();

    return instance_;
}

void RetainMemory::Release()
{
    if (instance_) {
        delete instance_;
        instance_ = NULL;
    }
}

void RetainMemory::AddToOperandsStack(std::shared_ptr<TOperandsStack>& stackRef)
{
    operandsRetainStack.push_back(stackRef);
}

void RetainMemory::AddToDictsStack(std::shared_ptr<TDictionaries>& stackRef)
{
    dictsRetainStack.push_back(stackRef);
}

void RetainMemory::AddToResStack(std::shared_ptr<ResourceData>& stackRef)
{
    resRetainStack.push_back(stackRef);
}

void RetainMemory::AddToRefObjStack(RefCountPtr<PDFObject>& stackRef)
{
    refObjRetainStack.push_back(stackRef);
}

