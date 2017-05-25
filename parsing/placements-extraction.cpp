#include "placements-extraction.h"
#include "pdf-interpreter.h"
#include "PDFReal.h"
#include "PDFIndirectObjectReference.h"
#include "ParsedPrimitiveHelper.h"
#include <iostream>
#include "RetainMemory.h"

std::shared_ptr<ResourceData> ParseInterestingResources(std::shared_ptr<TDictionaries> resourcesDicts, PDFParser* pdfReader, TReadResourcesFn readResources) {

    std::shared_ptr<ResourceData> result = std::make_shared<ResourceData>();

    if (resourcesDicts.get()->size()) {

        if (MultiDictHelperExists("XObject", resourcesDicts.get())) {

            PDFObjectCastPtr<PDFDictionary> xobjects(MultiDictHelperQueryDictionaryObject("XObject", pdfReader, resourcesDicts.get()).GetPtr());

            if(xobjects.GetPtr()) {
                xobjects->AddRef();

                RefCountPtr<PDFName> key;
                PDFObjectCastPtr<PDFIndirectObjectReference> value;
                MapIterator<PDFNameToPDFObjectMap> it = xobjects->GetIterator();

                while(it.MoveNext())
                {
                    key = it.GetKey();
                    value = it.GetValue();

                    PDFObjectCastPtr<PDFStreamInput> xobject(pdfReader->ParseNewObject(value->mObjectID));

                    if (xobject.GetPtr()) {
                        PDFObjectCastPtr<PDFDictionary> xobjectDictionary(xobject->QueryStreamDictionary());
                        PDFObjectCastPtr<PDFName> typeOfXObject = xobjectDictionary->QueryDirectObject("Subtype");

                        std::string str1(typeOfXObject->GetValue());

                        if (typeOfXObject->GetValue() == std::string("Form")) {
                            FormItem form;

                            form.id = value->mObjectID;
                            form.xobject = xobject.GetPtr();

                            if (xobjectDictionary->Exists("Matrix")) {
                                //form.matrix
                                PDFObjectCastPtr<PDFArray> matrix = pdfReader->QueryDictionaryObject(xobjectDictionary.GetPtr(), "Matrix");
                                unsigned long len = matrix->GetLength();

                                for (unsigned long i = 0; i < len; ++i) {
                                    RefCountPtr<PDFObject> matrixNumberPtr(matrix->QueryObject(i));
                                    ParsedPrimitiveHelper mNumber(matrixNumberPtr.GetPtr());
                                    form.matrix[i] = mNumber.GetAsDouble();
                                }
                            }

                            result.get()->forms.insert(std::make_pair(key->GetValue(), form));
                        }
                    }
                }
            }
        }

        if (readResources) {
            readResources(resourcesDicts, pdfReader, *(result.get()));
        }
    }

    return result;
}

// gets an array of resources dictionaries, going up parents. should
// grab 1 for forms, and 1 or more for pages
std::shared_ptr<TDictionaries> GetResourcesDictionaries(RefCountPtr<PDFDictionary> anObject, PDFParser* pdfReader) {

    std::shared_ptr<TDictionaries> resourcesDicts = std::make_shared<TDictionaries>();
    RetainMemory::Instance()->AddToDictsStack(resourcesDicts);

    while (true) {

        if (anObject->Exists("Resources")) {
            PDFObjectCastPtr<PDFDictionary> dict(pdfReader->QueryDictionaryObject(anObject.GetPtr(), "Resources"));
            resourcesDicts->push_back(dict);
        }

        if (anObject->Exists("Parent")) {
            PDFObjectCastPtr<PDFDictionary> parentDict(pdfReader->QueryDictionaryObject(anObject.GetPtr(), "Parent"));

            if (parentDict.GetPtr() != NULL) {
                anObject = parentDict;
            } else {
                break;
            }

        } else {
            break;
        }
    }

    return resourcesDicts;
}

// inspect one page
void InspectPage(PDFParser* pdfReader, TCollectPlacementsFn collectPlacements, TReadResourcesFn readResources, TPlacements& placements, unsigned int pgNum)
{
    RefCountPtr<PDFDictionary> pageDictionary(pdfReader->ParsePage(pgNum));

    std::shared_ptr<ResourceData> resources = ParseInterestingResources(GetResourcesDictionaries(pageDictionary, pdfReader), pdfReader, readResources);
    RetainMemory::Instance()->AddToResStack(resources);

    InterpretPageContents(pdfReader, pageDictionary, collectPlacements (
        *(resources.get()),
        placements/*,
        *(formsUsed.get())*/
    ));

    // free page resources
    RetainMemory::Instance()->Release();

}
// iterate al pages at once
std::shared_ptr<TFormsUsed> InspectPages(PDFParser* pdfReader, TCollectPlacementsFn collectPlacements, TReadResourcesFn readResources, TPagesPlacements& pagesPlacements)
{
    std::shared_ptr<TFormsUsed> formsUsed = std::make_shared<TFormsUsed>();

    // iterate pages, fetch placements, and mark forms for later additional inspection
    for (unsigned int i = 0; i < pdfReader->GetPagesCount(); ++i) {

        RefCountPtr<PDFDictionary> pageDictionary(pdfReader->ParsePage(i));

        TPlacements placements;
        std::shared_ptr<ResourceData> resources = ParseInterestingResources(GetResourcesDictionaries(pageDictionary, pdfReader), pdfReader, readResources);
        RetainMemory::Instance()->AddToResStack(resources);

        InterpretPageContents(pdfReader, pageDictionary, collectPlacements (
            *(resources.get()),
            placements/*,
            *(formsUsed.get())*/
        ));

        pagesPlacements.push_back(placements);
        // free page resources
        RetainMemory::Instance()->Release();
    }

    return formsUsed;
}


TFormsBackLog& InspectForms(TFormsUsed& formsToProcess, PDFParser* pdfReader, TFormsBackLog& formsBacklog,
                            TCollectPlacementsFn collectPlacements, TReadResourcesFn readResources)
{

    if (formsToProcess.size() == 0) {
        return formsBacklog;
    }

    // add fresh entries to backlog for the sake of registering the forms as discovered,
    // and to provide structs for filling with placement data...

    TFormsUsed formsUsed;

    for (const auto &formIdPair : formsToProcess) {

        PDFObjectCastPtr<PDFDictionary> dict(((PDFStreamInput*)formIdPair.second.GetPtr())->QueryStreamDictionary());

        if (!dict.GetPtr()) {
            std::cerr << "Empty stream" << std::endl;
        }

        std::shared_ptr<ResourceData> resources = ParseInterestingResources(GetResourcesDictionaries(dict, pdfReader), pdfReader, readResources);
        RetainMemory::Instance()->AddToResStack(resources);

        PDFObjectCastPtr<PDFStreamInput> stream(formIdPair.second.GetPtr());
        stream->AddRef();

        InterpretXObjectContents(pdfReader, stream, collectPlacements (
            *(resources.get()),
            formsBacklog[formIdPair.first]/*, // ... formsBacklog extended here if key from formsToProcess is not exist for formsBacklog
            formsUsed*/
        ));
    }

    TFormsUsed newUsedForms;
    bool keyExist = false;

    for (const auto& formsUsedPair: formsUsed) {
        for (const auto& backlogPair: formsBacklog) {
            if (backlogPair.first == formsUsedPair.first) {
                keyExist = true;
                break;
            }
        }

        if (!keyExist) {
            newUsedForms.insert(formsUsedPair);
        } else {
            keyExist = false;
        }
    }

    // recurse to new forms
    InspectForms(newUsedForms, pdfReader, formsBacklog, collectPlacements, readResources);

    // return final result
    return formsBacklog;
}

void ExtractPlacements(PDFParser* pdfReader, TCollectPlacementsFn collectPlacements, TReadResourcesFn readResources, TPagesPlacements& pagesPlacements, TFormsBackLog& formsPlacements)
{
    std::shared_ptr<TFormsUsed> formsUsed = InspectPages(pdfReader, collectPlacements, readResources, pagesPlacements);

    //InspectForms(*(formsUsed.get()), pdfReader, formsPlacements, collectPlacements, readResources);
}
