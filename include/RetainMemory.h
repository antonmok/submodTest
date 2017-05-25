#pragma once

#include <vector>
#include "RefCountPtr.h"
#include "PDFObject.h"
#include <memory>
#include "multi-dict-helper.h"
#include "pdf-interpreter.h"
#include "placements-extraction.h"

class RetainMemory
{
public:
    RetainMemory();
    ~RetainMemory();

    static RetainMemory* Instance();
    static void Release();

    void AddToOperandsStack(std::shared_ptr<TOperandsStack>&);
    void AddToDictsStack(std::shared_ptr<TDictionaries>&);
    void AddToResStack(std::shared_ptr<ResourceData>&);
    void AddToRefObjStack(RefCountPtr<PDFObject>&);

private:
    static RetainMemory* instance_;
    std::vector<std::shared_ptr<TOperandsStack>> operandsRetainStack;
    std::vector<std::shared_ptr<TDictionaries>> dictsRetainStack;
    std::vector<std::shared_ptr<ResourceData>> resRetainStack;
    std::vector<RefCountPtr<PDFObject>> refObjRetainStack;
};
