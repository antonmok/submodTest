#include "pdf-interpreter.h"
#include "PDFSymbol.h"
#include "PDFObjectCast.h"
#include "RetainMemory.h"
#include <unordered_set>

std::unordered_set<std::string> opSet = { "q", "Q", "cm", "gs", "Do", "Tc", "Tw", "Tz", "TL", "Ts", "Tf", "BT", "ET", "Td", "TD", "Tm", "T*", "Tj", "\'", "\"", "TJ" };

/*double GetTickCount(void)
{
  struct timespec now;
  if (clock_gettime(CLOCK_MONOTONIC, &now))
    return 0;
  return now.tv_sec * 1000.0 + now.tv_nsec / 1000000.0;
}*/

 std::shared_ptr<TOperandsStack> InterpretContentStream(PDFParser* pdfReader, RefCountPtr<PDFStreamInput> contentStream, TOperatorHandler onOperatorHandler,
                                                        std::shared_ptr<TOperandsStack> operandStackInit, bool filter = false)
{
    std::string opName;
    std::shared_ptr<TOperandsStack> operandsStack;

    PDFObjectParser* objectParser = pdfReader->StartReadingObjectsFromStream(contentStream.GetPtr());

    if (operandStackInit) {
        operandsStack = operandStackInit;
    } else {
        operandsStack = std::make_shared<TOperandsStack>();
        RetainMemory::Instance()->AddToOperandsStack(operandsStack);
    }

    while (true) {

        RefCountPtr<PDFObject> anObject(objectParser->ParseNewObject());

        if (!anObject.GetPtr()) break;

        if (anObject->GetType() == PDFObject::ePDFObjectSymbol) {    // operator (Tw, Td etc)

            opName.assign(((PDFSymbol*)anObject.GetPtr())->GetValue());
            if (filter && !opSet.count(opName)) {                              // filter some operands
                operandsStack = std::make_shared<TOperandsStack>();
                RetainMemory::Instance()->AddToOperandsStack(operandsStack);
                continue;
            }

            // copy stack
            std::shared_ptr<TOperandsStack> operandsStackCopyPtr = std::make_shared<TOperandsStack>(*(operandsStack.get()));
            RetainMemory::Instance()->AddToOperandsStack(operandsStackCopyPtr);

            onOperatorHandler(opName, operandsStackCopyPtr);
            operandsStack = std::make_shared<TOperandsStack>();
            RetainMemory::Instance()->AddToOperandsStack(operandsStack);

        } else {    // operand
            operandsStack->push_back(anObject);
        }
    }

    delete objectParser;

    return operandsStack;
}

void InterpretPageContents(PDFParser* pdfReader, RefCountPtr<PDFDictionary> pageObject, TOperatorHandler onOperatorHandler) {

    if (pageObject->GetType() != PDFObject::ePDFObjectDictionary) {
        return;
    }

    if (!pageObject->Exists("Contents"))
        return;

    RefCountPtr<PDFObject> contents(pdfReader->QueryDictionaryObject(pageObject.GetPtr(), "Contents"));
    RetainMemory::Instance()->AddToRefObjStack(contents);

    if (contents.GetPtr()->GetType() == PDFObject::ePDFObjectArray) {

        contents.GetPtr()->AddRef(); // add ref before creating new ref container
        PDFObjectCastPtr<PDFArray> contentsArray(contents.GetPtr());

        std::shared_ptr<TOperandsStack> carriedOperandsStack = std::make_shared<TOperandsStack>();
        RetainMemory::Instance()->AddToOperandsStack(carriedOperandsStack);

        for (unsigned long i = 0; i < contentsArray.GetPtr()->GetLength(); ++i) {
            PDFObjectCastPtr<PDFStreamInput> stream(pdfReader->QueryArrayObject(contentsArray.GetPtr(), i));
            carriedOperandsStack = InterpretContentStream(pdfReader, stream, onOperatorHandler, carriedOperandsStack, true);
        }

    } else {
        contents.GetPtr()->AddRef(); // add ref before creating new ref container
        PDFObjectCastPtr<PDFStreamInput> contentsStream(contents.GetPtr());
        InterpretContentStream(pdfReader, contentsStream, onOperatorHandler, nullptr, true);
    }
}

void InterpretXObjectContents(PDFParser* pdfReader, RefCountPtr<PDFStreamInput> xobjectObject, TOperatorHandler onOperatorHandler) {
    InterpretContentStream(pdfReader, xobjectObject, onOperatorHandler, nullptr);
}

void InterpretStream(PDFParser* pdfReader, RefCountPtr<PDFStreamInput> stream, TOperatorHandler onOperatorHandler) {
    InterpretContentStream(pdfReader, stream, onOperatorHandler, nullptr);
}
