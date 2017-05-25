#pragma once
#include "ParsedPDFData.h"

enum APP_CMD_TYPE {
    EXTRACT = 0,
    INSERT_LINK,
    OPTIMIZE,
    VALIDATE
};

struct SArgsData {
    std::string filePath;
    std::string resultDirPath;
    APP_CMD_TYPE cmd;
    std::string replUrl;
    std::string regExp;
    unsigned int paddLen;
    bool verbose;
};

int PerformAppCmd(const SArgsData& argsData);
