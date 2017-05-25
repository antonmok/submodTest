#pragma once

#include <functional>
#include <vector>
#include <map>
#include "PDFParser.h"
#include "ObjectsBasicTypes.h"
#include "PDFDictionary.h"
#include "PDFObjectCast.h"
#include "ParsedPrimitiveHelper.h"
#include "PDFIndirectObjectReference.h"
#include "collection-state.h"
#include <unordered_map>

typedef std::function<void(double, unsigned char)> TCharsIterator;
typedef std::map<uint, std::vector<uint>> TUnicodeMap;
typedef std::unordered_map<uint, std::string> TEncodingMap;

class CFontDecoding {

public:
    CFontDecoding();
    CFontDecoding(PDFParser*, ObjectIDType);
    ~CFontDecoding();

    void Translate(STextInput&);
    void IterateTextDisplacements(std::vector<unsigned char>& encodedBytes, TCharsIterator CharsIterator);

    signed int ascent;
    signed int descent;

private:
    void ParseSimpleFontEncoding(PDFParser* pdfReader, PDFObjectCastPtr<PDFDictionary> font, RefCountPtr<PDFObject> encoding);
    void ParseSimpleFontDimensions(PDFParser* pdfReader, PDFObjectCastPtr<PDFDictionary> font);
    void ParseCIDFontDimensions(PDFParser* pdfReader, PDFObjectCastPtr<PDFDictionary> font);
    TEncodingMap* SetupDifferencesEncodingMap(PDFParser* pdfReader, PDFObjectCastPtr<PDFDictionary> font, PDFObjectCastPtr<PDFDictionary> encodingDict);
    double GetDecoderWidth(unsigned char code);

    bool isSimpleFont;
    bool hasSimpleEncoding;
    std::unordered_map<signed int, signed int> widths;
    double defaultWidth;
    bool hasToUnicode;

    TUnicodeMap toUnicodeMap;
    TEncodingMap* fromSimpleEncodingMap;
    TEncodingMap diffEncodingMap;

};

typedef std::map<ObjectIDType, CFontDecoding> TFontDecoders;
