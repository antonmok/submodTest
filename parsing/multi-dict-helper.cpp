#include "multi-dict-helper.h"
#include "RetainMemory.h"

bool MultiDictHelperExists(std::string name, TDictionaries* dictionaries) {

    for (const auto& dictItem: *dictionaries) {
        if (dictItem.GetPtr()->Exists(name)) {
            return true;
        }
    }

    return false;
}

RefCountPtr<PDFObject> MultiDictHelperQueryDictionaryObject(std::string name, PDFParser* pdfReader, TDictionaries* dictionaries) {

    for (const auto& dictItem: *dictionaries) {
        if (dictItem.GetPtr()->Exists(name)) {
            RefCountPtr<PDFObject> retObj(pdfReader->QueryDictionaryObject(dictItem.GetPtr(), name));
            RetainMemory::Instance()->AddToRefObjStack(retObj);

            return retObj;
        }
    }

    return nullptr;
}
