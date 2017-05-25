#pragma once

#include <string>
#include <vector>
#include <memory>

#include "PDFDictionary.h"
#include "PDFParser.h"
#include "RefCountPtr.h"


typedef std::vector<RefCountPtr<PDFDictionary>> TDictionaries;

bool MultiDictHelperExists(std::string name, TDictionaries* dictionaries);
RefCountPtr<PDFObject> MultiDictHelperQueryDictionaryObject(std::string name, PDFParser* pdfReader, TDictionaries* dictionaries);
