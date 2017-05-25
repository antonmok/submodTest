#pragma once

#include <vector>
#include <memory>
#include <functional>
#include "PDFParser.h"
#include "PDFObject.h"
#include "PDFArray.h"
#include "PDFStreamInput.h"
#include "RefCountPtr.h"
#include "PDFObjectCast.h"

typedef std::vector<RefCountPtr<PDFObject>> TOperandsStack;
typedef std::function<void(const std::string&, std::shared_ptr<TOperandsStack>)> TOperatorHandler;

void InterpretPageContents(PDFParser* pdfReader, RefCountPtr<PDFDictionary> pageObject, TOperatorHandler onOperatorHandler);
void InterpretXObjectContents(PDFParser* pdfReader, RefCountPtr<PDFStreamInput> xobjectObject, TOperatorHandler onOperatorHandler);
void InterpretStream(PDFParser* pdfReader, RefCountPtr<PDFStreamInput> stream, TOperatorHandler onOperatorHandler);
