#include "CmdHandler.h"
#include "pdfParser.h"
#include "SaveText.h"
#include "InsertLink.h"
#include <iostream>

int PerformAppCmd(const SArgsData& argsData)
{
    ParsedPDFDataListPerPage parsedPDFDataListPerPage;
    unsigned int pgCnt = 0;
    int errCode = 0;

    if (argsData.verbose) {
        std::cout << "Prasing PDF...\n\n" ;
    }

    // perform cmd
    switch (argsData.cmd) {

        case APP_CMD_TYPE::INSERT_LINK:

            pgCnt = ParsePDF(argsData.filePath, parsedPDFDataListPerPage, errCode);
            if (errCode != 0) {
                return errCode;
            }
            // search SKU pattern
            InsertLinkOverSKUNumbers(parsedPDFDataListPerPage, argsData);
            break;

        case APP_CMD_TYPE::OPTIMIZE:
            //
            break;

        case APP_CMD_TYPE::VALIDATE:
            errCode = ValidatePDF(argsData.filePath);
            break;

        case APP_CMD_TYPE::EXTRACT:

            pgCnt = ParsePDF(argsData.filePath, parsedPDFDataListPerPage, errCode);
            if (errCode != 0) {
                return errCode;
            }
            // safe pdf text to txt files per page
            for (unsigned int i = 0; i < pgCnt; ++i) {
                SafePageText(parsedPDFDataListPerPage.at(i), i, argsData.resultDirPath);
            }
            break;
    }

    return errCode;
}
