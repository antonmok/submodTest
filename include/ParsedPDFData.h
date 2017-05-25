#pragma once

#include <vector>
#include <string>
#include "collection-state.h"

struct ParsedPDFData {
    ParsedPDFData() {
        lastItemInBlock = false;
    }
    bool lastItemInBlock;
    std::string text; // string - the text
    double matrix[6]; // numbers[6] - 6 numbers pdf matrix describing how the text is transformed in relation to the page (this includes position - translation)
    double localBBox[4]; // numbers[4] 4 numbers box describing the text bounding box, before being transformed by matrix.
    double globalBBox[4]; // numbers[4] 4 numbers box describing the text bounding box after transoformation, making it the bbox in relation to the page.
};

typedef std::vector<ParsedPDFData> ParsedPDFDataList;
typedef std::vector<ParsedPDFDataList> ParsedPDFDataListPerPage;

