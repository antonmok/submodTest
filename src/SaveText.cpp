#include "SaveText.h"
#include "PDFParser.h"
#include <iostream>
#include <cmath>
#include <fstream>

#define NEW_LINE_THRESHOLD 5
#define WORD_BREAK_THRESHOLD 5
#define TEXT_BLOCKS_GAP_THRESHOLD 2

bool PrevFalseFragmented(double xCoordPrev, double widthPrev, double xCoordCur, double yCoordPrev, double yCoordCur) {

    double x2CoordPrev = xCoordPrev + widthPrev;

    if (yCoordCur == yCoordPrev && (x2CoordPrev > xCoordCur - TEXT_BLOCKS_GAP_THRESHOLD && x2CoordPrev < xCoordCur + TEXT_BLOCKS_GAP_THRESHOLD)) {
        return true;
    }

    return false;
}

void BuildTextForPage(std::string& outBuf, const ParsedPDFDataList& parsedPDFDataList) {

    double yCoordCur = 0;
    double yCoordPrev = 0;
    double xCoordCur = 0;
    double xCoordPrev = 0;
    double widthCur = 0;
    double widthPrev = 0;
    bool firstRun = true;
    bool lastItemInBlockPrev = false;

    for (const ParsedPDFData& parsedData : parsedPDFDataList) {

        yCoordCur = parsedData.globalBBox[1];
        xCoordCur = parsedData.globalBBox[0];
        widthCur = parsedData.globalBBox[2] - parsedData.globalBBox[0];

        if (lastItemInBlockPrev && !PrevFalseFragmented(xCoordPrev, widthPrev, xCoordCur, yCoordPrev, yCoordPrev)) {
            outBuf.append("\n");
        }

        // detect new line
        if (!firstRun && abs(yCoordPrev - yCoordCur) > NEW_LINE_THRESHOLD) {
            outBuf.append("\n");
        } else {
            // detect space between words
            if (xCoordCur > xCoordPrev) { // if consequent
                if (!firstRun && xCoordCur - (xCoordPrev + widthPrev) > WORD_BREAK_THRESHOLD) {
                    outBuf.append(" ");
                }
            } else {
                if (!firstRun && xCoordPrev - (xCoordCur + widthCur) > WORD_BREAK_THRESHOLD) {
                    outBuf.append(" ");
                }
            }
        }

        outBuf.append(parsedData.text);

        if (parsedData.lastItemInBlock) {
            lastItemInBlockPrev = true;
        } else {
            lastItemInBlockPrev = false;
        }

        yCoordPrev = yCoordCur;
        xCoordPrev = xCoordCur;
        widthPrev = widthCur;

        firstRun = false;
    }
}

std::wofstream& operator<<(std::wofstream& stream, std::string& str)
{
    stream << str.c_str();
    return stream;
}

void SafePageText(ParsedPDFDataList& parsedPDFData, unsigned int pgNum, std::string resDirPath) {
    std::string accText;

    BuildTextForPage(accText, parsedPDFData);

    std::ofstream outFile;
    outFile.open(resDirPath + std::string("/page_") + std::to_string(pgNum) + std::string(".txt"), std::ios::out|std::ios::binary);

    if (outFile.is_open()) {
        outFile << accText;
        outFile.close();
        std::cout << "page_" << std::to_string(pgNum) + std::string(".txt created") << std::endl;
    } else {
        std::cerr << "file not found: " + resDirPath + std::string("/page_") + std::to_string(pgNum) + std::string(".txt")  << std::endl;
    }

    std::flush(std::cout);
}
