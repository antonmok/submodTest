#pragma once

#include "ParsedPDFData.h"

unsigned int ParsePDF(const std::string& file, ParsedPDFDataListPerPage& parsedDataListPerPg, int& errCode);
int ValidatePDF(const std::string& filePath);
