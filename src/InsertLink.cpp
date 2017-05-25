#include "InsertLink.h"
#include "PDFParser.h"
#include <regex>
#include <algorithm>
#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include "PDFRectangle.h"
#include "PDFWriter.h"
#include "PDFModifiedPage.h"

struct SLinkToInsert {
    PDFRectangle rect;
    std::string prodUrl;
    unsigned int pgNum;
};

bool FindSKUNumberCoords(ParsedPDFData& parsedPDFData, PDFRectangle& rect, std::string& SKUNumStr, const std::string& regExp) {

    std::smatch sm;
    std::regex regExpObj(regExp);

    if (std::regex_search (parsedPDFData.text, sm, regExpObj)) {
        SKUNumStr = sm.str();
        rect.LowerLeftX = parsedPDFData.globalBBox[0];
        rect.LowerLeftY = parsedPDFData.globalBBox[1];
        rect.UpperRightX = parsedPDFData.globalBBox[2];
        rect.UpperRightY = parsedPDFData.globalBBox[3];

        return true;
    }

    return false;
}

void InsertLinks(const std::string& fileFullPath, const std::string& newFileFullPath, const std::vector<SLinkToInsert>& linksToInsert) {

    PDFWriter pdfWriter;
    pdfWriter.ModifyPDF(fileFullPath, ePDFVersionMax, newFileFullPath);

    PDFModifiedPage* modifiedPage = nullptr;
    unsigned int curPgNum = 0;
    bool firstPage = true;

    for (SLinkToInsert lnk: linksToInsert) {

        if (firstPage) {
            modifiedPage = new PDFModifiedPage(&pdfWriter, lnk.pgNum);
            curPgNum = lnk.pgNum;
            firstPage = false;
        } else {
            if (curPgNum != lnk.pgNum) {
                curPgNum = lnk.pgNum;
                if (modifiedPage->WritePage() != eSuccess) {
                    std::cerr << "failed in WritePage, pgNum: " << lnk.pgNum << "\n";
                }

                delete modifiedPage;
                modifiedPage = new PDFModifiedPage(&pdfWriter, lnk.pgNum);
            }
        }

        if (modifiedPage->AttachURLLinktoCurrentPage(lnk.prodUrl, lnk.rect) != eSuccess) {
            std::cerr << "failed in AttachURLLinktoCurrentPage, pgNum: " << lnk.pgNum << "\n";
        }
    }

    if (modifiedPage->WritePage() != eSuccess) {
        std::cerr << "failed in WritePage, pgNum: " << curPgNum << "\n";
    }

    delete modifiedPage;

    if (pdfWriter.EndPDF()!= eSuccess) {
        std::cerr << "failed in end PDF\n";
    }
}

void RestoreLinearizedPDF(const std::string& filePath, bool verbose) {    // -dUseCropBox -dFastWebView=true  -dPDFSETTINGS=/printer prepress

    std::string command("gs -o " + filePath + "_" + " -sDEVICE=pdfwrite -dUseCropBox -dCompatibilityLevel=1.6 -dPDFSETTINGS=/printer " + filePath);// + " > /dev/null");

    if (!verbose) {
        command.append(" > /dev/null");
    }

    if (system(command.c_str()) == -1) {
        std::cerr << "failed in RestoreLinearizedPDF(): 1" << "\n";
    }

    command = "rm " + filePath;
    if (system(command.c_str()) == -1) {
        std::cerr << "failed in RestoreLinearizedPDF(): 2" << "\n";
    }

    command = "mv " + filePath + "_ " + filePath;
    if (system(command.c_str()) == -1) {
        std::cerr << "failed in RestoreLinearizedPDF(): 3" << "\n";
    }
}

void InsertLinkOverSKUNumbers(ParsedPDFDataListPerPage& parsedPDFDataListDoc, const SArgsData& argsData) {

    std::vector<SLinkToInsert> linksToInsert;
    std::string SkuNumStr;
    std::string replUrl(argsData.replUrl);
    std::string replUrlFirst;
    std::string replUrlLast;
    std::string placeholder = "{placeholder}";
    PDFRectangle rect;
    unsigned int pgNum = 0;
    unsigned int linksCnt = 0;

    // prepare replUrl parts
    std::size_t pos = replUrl.find(placeholder);
    if (pos == std::string::npos) {
        std::cerr << "Wrong placeholder format\n";
        return;
    }
    replUrlFirst = replUrl.substr(0, pos);
    replUrlLast = replUrl.substr(pos + placeholder.size());

    // find all places for link insertion
    for (ParsedPDFDataList& parsedPDFDataList: parsedPDFDataListDoc) {
        for (ParsedPDFData& parsedPDFData : parsedPDFDataList) {
            if (FindSKUNumberCoords(parsedPDFData, rect, SkuNumStr, argsData.regExp)) {
                // prepare link
                SkuNumStr.erase(std::remove(SkuNumStr.begin(), SkuNumStr.end(), '.'), SkuNumStr.end());
                std::string fillerZeros(argsData.paddLen - SkuNumStr.size(), '0');
                std::string prodUrl = replUrlFirst + fillerZeros + SkuNumStr + replUrlLast;
                linksToInsert.push_back({rect, prodUrl, pgNum});

                if (argsData.verbose) {
                    std::cout << "Link " << prodUrl << " inserted on page " << pgNum << std::endl;
                    linksCnt++;
                }
            }
        }

        if (argsData.verbose && linksCnt) {
            std::cout << linksCnt << " links inserted on page " << pgNum << "\n\n";
            linksCnt = 0;;
        }

        pgNum++;
    }

    // prepare files paths
    std::string pdfFileName = argsData.filePath.substr(argsData.filePath.find_last_of("/") + 1);
    std::string newFileFullPath = argsData.resultDirPath + "/" + pdfFileName;
    struct stat st;

    if (!stat(newFileFullPath.c_str(), &st)) {
        if (S_ISREG(st.st_mode)) {
            // alter file name if in the same folder
            newFileFullPath = argsData.resultDirPath + "/modified_" + pdfFileName;
        }
    }

    InsertLinks(argsData.filePath, newFileFullPath, linksToInsert);

    if (argsData.verbose) {
        std::cout << "Inserted " << linksToInsert.size() << " links total\n\n";
    }

    RestoreLinearizedPDF(newFileFullPath, argsData.verbose);
}
