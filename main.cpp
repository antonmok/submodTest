
#include "PDFWriter.h"
#include "tclap/CmdLine.h"
#include <string>
#include "CmdHandler.h"
#include <sys/stat.h>

#define APP_CMD_STR_EXTRACT "extract"
#define APP_CMD_STR_INSERT "insert"
#define APP_CMD_STR_VERIFY "verify"

//#include "test/BasicModification.h"

bool FileExist(const std::string& path) {
    struct stat st;
    stat(path.c_str(), &st);

    if (!S_ISREG(st.st_mode)) {
        return false;
    }

    return true;
}

bool DirExist(const std::string& path) {
    struct stat st;
    stat(path.c_str(), &st);

    if (!S_ISDIR(st.st_mode)) {
        return false;
    }

    return true;
}

int main(int argc, char** argv)
{
    std::string pdfFilePath;
    std::string appCmd;
    std::string resultsDirPath;
    std::string regExp;
    std::string replUrl;
    unsigned int paddLen;
    bool verboseFlag;

    try {
        TCLAP::CmdLine cmd("Parse pdf utility", ' ', "0.1");
        TCLAP::ValueArg<std::string> pathArg("p", "pdfPath", "Path to PDF file", true, "", "string");
        TCLAP::ValueArg<std::string> cmdArg("c", "command", "Operation to perform.\n'extract' - extract text to txt files\n'insert' - insert link over SKU number", true, "", "string");
        TCLAP::ValueArg<std::string> outPathArg("o", "outPath", "Output directory with processing results", false, "", "string");
        TCLAP::ValueArg<std::string> regExpArg("r", "regExp", "Regular expression for search, required for -c insert", false, "", "string");
        TCLAP::ValueArg<std::string> replUrlArg("u", "replUrl", "Url to place over SKU, required for -c insert", false, "", "string");
        TCLAP::ValueArg<unsigned int> paddLenArg("l", "paddLen", "Padded placeholder length, required for -c insert", false, 0, "integer");
        TCLAP::SwitchArg verboseFlagArg("v", "verbose", "Output additional processing info");

        cmd.add(pathArg);
        cmd.add(cmdArg);
        cmd.add(outPathArg);
        cmd.add(regExpArg);
        cmd.add(replUrlArg);
        cmd.add(paddLenArg);
        cmd.add(verboseFlagArg);

        cmd.parse(argc, argv);

        pdfFilePath = pathArg.getValue();
        appCmd = cmdArg.getValue();
        resultsDirPath = outPathArg.getValue();

        paddLen = paddLenArg.getValue();
        replUrl = replUrlArg.getValue();
        regExp = regExpArg.getValue();
        verboseFlag = verboseFlagArg.getValue();

        // validate input file path
        if (!FileExist(pdfFilePath)) {
            std::cerr << "error: input file not found\n";
            return 2;
        }

    } catch (TCLAP::ArgException &e) {
        std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
        return 3;
    }

    SArgsData argsData;

    argsData.filePath = pdfFilePath;
    argsData.resultDirPath = resultsDirPath;
    argsData.paddLen = paddLen;
    argsData.regExp = regExp;
    argsData.replUrl = replUrl;
    argsData.verbose = verboseFlag;

    if (appCmd == APP_CMD_STR_VERIFY) {

        argsData.cmd = APP_CMD_TYPE::VALIDATE;

    } else if (appCmd == APP_CMD_STR_EXTRACT) {

        // validate output path
        if (!DirExist(resultsDirPath)) {
            std::cerr << "error: output dir not exist\n";
            return 1;
        }
        argsData.cmd = APP_CMD_TYPE::EXTRACT;

    } else if (appCmd == APP_CMD_STR_INSERT) {

        // validate output path
        if (!DirExist(resultsDirPath)) {
            std::cerr << "error: output dir not exist\n";
            return 1;
        }

        // check optional args
        if (paddLen == 0 || replUrl == "" || regExp == "") {
            std::cerr << "Missing arguments -r, -u or -l" << std::endl;
            return 3;
        }

        argsData.cmd = APP_CMD_TYPE::INSERT_LINK;
    } else {
        std::cerr << "Wrong operation name. Use 'extract' or 'insert'" << std::endl;
        return 3;
    }

    return PerformAppCmd(argsData);
}
