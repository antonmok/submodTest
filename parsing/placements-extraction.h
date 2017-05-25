#pragma once

#include "PDFParser.h"
#include "pdf-interpreter.h"
#include "multi-dict-helper.h"

#include <vector>
#include <functional>
#include <map>
#include <memory>

#include "collection-state.h"

struct ExtGStateItem {
    ExtGStateItem() {
        fontRef = 0;
        size = 0;
    };

    RefCountPtr<PDFObject> theObject;
    ObjectIDType fontRef;
    unsigned long size;
};

struct FormItem {
    ObjectIDType id;
    RefCountPtr<PDFObject> xobject;
    float matrix[6];
};

struct ResourceData {
    std::map<std::string, ExtGStateItem> extGStates;
    std::map<std::string, FormItem> forms;
    std::map<std::string, ObjectIDType> fonts;
};

typedef std::vector<TPlacements> TPagesPlacements;
typedef std::map<ObjectIDType, RefCountPtr<PDFObject>> TFormsUsed;
typedef std::map<ObjectIDType, TPlacements> TFormsBackLog;

typedef std::function<void(std::shared_ptr<TDictionaries> resourcesDicts, PDFParser* pdfReader, ResourceData& result)> TReadResourcesFn;
typedef std::function<TOperatorHandler(ResourceData& resources, TPlacements& placements/*, TFormsUsed& formsUsed*/)> TCollectPlacementsFn;

void ExtractPlacements(PDFParser* pdfReader, TCollectPlacementsFn collectPlacements, TReadResourcesFn readResources, TPagesPlacements& pagesPlacements, TFormsBackLog& formsPlacements);
void InspectPage(PDFParser* pdfReader, TCollectPlacementsFn collectPlacements, TReadResourcesFn readResources, TPlacements& placements, unsigned int pgNum);
