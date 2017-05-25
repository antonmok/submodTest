#include "font-decoding.h"
#include "pdf-interpreter.h"
#include "PDFArray.h"
#include "PDFName.h"
#include <regex>

#include "adobe-glyph-list.h"
#include "win-ansi-encoding.h"
#include "mac-expert-encoding.h"
#include "mac-roman-encoding.h"
#include "symbol-encoding.h"
#include "standard-encoding.h"
#include "standard-fonts-dimensions.h"

uint BeToNum(const std::string& inArray, uint start = 0, uint end = 0);

void BesToUnicodes(std::string inArray, std::vector<uint>& unicodes) {
    uint i = 0;

    while (i < inArray.size()) {
        uint newOne = BeToNum(inArray, i, i + 2);
        if (0xD800 <= newOne && newOne <= 0xDBFF) {
            // pfff. high surrogate. need to read another one
            i += 2;
            uint lowSurrogate = BeToNum(inArray, i, i + 2);
            unicodes.push_back(0x10000 + ((newOne - 0xD800) << 10) + (lowSurrogate - 0xDC00));

        } else {
            unicodes.push_back(newOne);
        }

        i += 2;
    }
}

uint BeToNum(const std::string& inArray, uint start, uint end) {
    uint result = 0;

    if (end == 0) {
        end = inArray.size();
    }

    for (uint i = start; i < end; ++i) {
        result = result * 256 + (unsigned char)inArray[i];
    }

    return result;
}

void ParseToUnicode(PDFParser* pdfReader, ObjectIDType toUnicodeObjectId, TUnicodeMap& uMap) {

    // yessss! to unicode map. we can read it rather nicely
    // with the interpreter class looking only for endbfrange and endbfchar as "operands"
    PDFObjectCastPtr<PDFStreamInput> stream(pdfReader->ParseNewObject(toUnicodeObjectId));

    if(!stream.GetPtr())
        return;

    InterpretStream(pdfReader, stream, [&uMap](const std::string& operatorName, std::shared_ptr<TOperandsStack> operands) {

        if (operatorName == "endbfchar") {

            // Operators are pairs. always of the form <codeByte> <unicodes>
            for (std::size_t i = 0; i < operands.get()->size(); i += 2) {
                ParsedPrimitiveHelper val1(operands.get()->at(i).GetPtr());
                ParsedPrimitiveHelper val2(operands.get()->at(i + 1).GetPtr());

                std::string byteCode = val1.ToString();
                std::string uniStr = val2.ToString();
                std::vector<uint> unicodes;
                BesToUnicodes(uniStr, unicodes);
                uMap[BeToNum(byteCode)] = unicodes;
            }
        }
        else if(operatorName == "endbfrange") {

            // Operators are 3. two codesBytes and then either a unicode start range or array of unicodes
            for(std::size_t i = 0; i < operands.get()->size(); i += 3) {

                ParsedPrimitiveHelper val1(operands.get()->at(i).GetPtr());
                ParsedPrimitiveHelper val2(operands.get()->at(i + 1).GetPtr());
                ParsedPrimitiveHelper val3(operands.get()->at(i + 2).GetPtr());

                uint startCode = BeToNum(val1.ToString());
                uint endCode = BeToNum(val2.ToString());

                if (operands.get()->at(i + 2)->GetType() == PDFObject::ePDFObjectArray) {
                    PDFObjectCastPtr<PDFArray> unicodeArray(operands.get()->at(i + 2).GetPtr());
                    unicodeArray->AddRef();

                    // specific codes
                    for (uint j = startCode; j <= endCode; ++j) {
                        std::vector<uint> unicodes;
                        if (unicodeArray->GetLength() <= j) {
                           break;
                        }
                        ParsedPrimitiveHelper itemVal(unicodeArray->QueryObject(j));
                        BesToUnicodes(itemVal.ToString(), unicodes);
                        uMap[j] = unicodes;
                    }
                } else {
                    std::vector<uint> unicodes;
                    BesToUnicodes(val3.ToString(), unicodes);
                    // code range
                    for (uint j = startCode; j <= endCode; ++j) {
                        std::vector<uint> unicodesCpy;
                        unicodesCpy = unicodes;
                        uMap[j] = unicodesCpy;
                        // increment last unicode value
                        unicodes[unicodes.size() - 1] = unicodes[unicodes.size() - 1] + 1;
                    }
                }

            }
        }
    });

}

TEncodingMap* GetStandardEncodingMap(const std::string& encodingName) {
    // MacRomanEncoding, MacExpertEncoding, or WinAnsiEncoding
    if(encodingName == "WinAnsiEncoding") {
        return &WinAnsiEncoding;
    }

    if(encodingName == "MacExpertEncoding")
        return &MacExpertEncoding;

    if(encodingName == "MacRomanEncoding")
        return &MacRomanEncoding;

    return nullptr;
}

TEncodingMap* CFontDecoding::SetupDifferencesEncodingMap(PDFParser* pdfReader, PDFObjectCastPtr<PDFDictionary> font, PDFObjectCastPtr<PDFDictionary> encodingDict) {
    // k. got ourselves differences array. let's see.
    if (encodingDict->Exists("BaseEncoding")) {

        RefCountPtr<PDFObject> encName(pdfReader->QueryDictionaryObject(encodingDict.GetPtr(), "BaseEncoding"));
        ParsedPrimitiveHelper nameVal(encName.GetPtr());

        TEncodingMap* baseEncoding = GetStandardEncodingMap(nameVal.ToString());

        if (baseEncoding) {
            return baseEncoding;
        }
    }

    // no base encoding. use standard or symbol. i'm gonna use either standard encoding or symbol encoding.
    // i know the right thing is to check first the font native encoding...but that's too much of a hassle
    // so i'll take the shortcut and if it is ever a problem - improve
    if (font->Exists("FontDescriptor")) {
        PDFObjectCastPtr<PDFDictionary> fontDescriptor(pdfReader->QueryDictionaryObject(font.GetPtr(), "FontDescriptor"));
        // check font descriptor to determine whether this is a symbolic font. if so, use symbol encoding. otherwise - standard
        ParsedPrimitiveHelper flagsVal(pdfReader->QueryDictionaryObject(fontDescriptor.GetPtr(), "Flags"));
        long long flags = flagsVal.GetAsInteger();

        if (flags & (1<<2)) {
            return &SymbolEncoding;
        } else {
            return &StandardEncoding;
        }
    } else {
        // assume standard
        return &StandardEncoding;
    }

    // now apply differences
    if (encodingDict->Exists("Differences")) {

        PDFObjectCastPtr<PDFArray> differences(pdfReader->QueryDictionaryObject(encodingDict.GetPtr(), "Differences"));
        unsigned long i = 0;
        long long firstIndex = 0;

        while (i < differences->GetLength()) {

            RefCountPtr<PDFObject> anObj(differences->QueryObject(i));
            ParsedPrimitiveHelper objVal(anObj.GetPtr());

            if (anObj->GetType() == PDFObject::ePDFObjectName) {    // names, one for each index
                diffEncodingMap[firstIndex] = objVal.ToString();
                ++firstIndex;
            } else {    // first item is always a number
                firstIndex = objVal.GetAsInteger();
            }
            ++i;
        }
    }

    return &diffEncodingMap;
}

void CFontDecoding::ParseSimpleFontEncoding(PDFParser* pdfReader, PDFObjectCastPtr<PDFDictionary> font, RefCountPtr<PDFObject> encoding) {

    if(encoding->GetType() == PDFObject::ePDFObjectName) {
        PDFObjectCastPtr<PDFName> encName(encoding.GetPtr());
        encName->AddRef();

        fromSimpleEncodingMap = GetStandardEncodingMap(encName->GetValue());
        hasSimpleEncoding = true;
    }
    else if(encoding->GetType() == PDFObject::ePDFObjectIndirectObjectReference || encoding->GetType() == PDFObject::ePDFObjectDictionary) {
        // make sure we have a dict here
        if (encoding->GetType() == PDFObject::ePDFObjectIndirectObjectReference) {

            PDFObjectCastPtr<PDFIndirectObjectReference> objRef(encoding.GetPtr());
            objRef->AddRef();

            PDFObjectCastPtr<PDFDictionary> encDict(pdfReader->ParseNewObject(objRef->mObjectID));

            fromSimpleEncodingMap = SetupDifferencesEncodingMap(pdfReader, font, encDict);

        } else {
            PDFObjectCastPtr<PDFDictionary> encDict(encoding.GetPtr());
            encDict->AddRef();

            fromSimpleEncodingMap = SetupDifferencesEncodingMap(pdfReader, font, encDict);
        }


        hasSimpleEncoding = true;
    }
}

void CFontDecoding::ParseSimpleFontDimensions(PDFParser* pdfReader, PDFObjectCastPtr<PDFDictionary> font) {
    // read specified widths
    if(font->Exists("FirstChar") && font->Exists("LastChar") && font->Exists("Widths")) {

        RefCountPtr<PDFObject> firstCharObj(pdfReader->QueryDictionaryObject(font.GetPtr(), "FirstChar"));
        RefCountPtr<PDFObject> lastCharObj(pdfReader->QueryDictionaryObject(font.GetPtr(), "LastChar"));
        PDFObjectCastPtr<PDFArray> widthsArr(pdfReader->QueryDictionaryObject(font.GetPtr(), "Widths"));

        ParsedPrimitiveHelper firstCharVal(firstCharObj.GetPtr());
        ParsedPrimitiveHelper lastCharVal(lastCharObj.GetPtr());

        long long firstChar = firstCharVal.GetAsInteger();
        long long lastChar= lastCharVal.GetAsInteger();

        // store widths for specified glyphs
        for (long long i = firstChar; i <= lastChar && (i - firstChar) < widthsArr->GetLength(); ++i) {
            RefCountPtr<PDFObject> widthsItemObj(pdfReader->QueryArrayObject(widthsArr.GetPtr(), i - firstChar));
            ParsedPrimitiveHelper widthsItemVal(widthsItemObj.GetPtr());
            widths[i] = widthsItemVal.GetAsInteger();
        }
    } else {
        // wtf. probably one of the standard fonts. aha! [will also take care of ascent descent]
        if (font->Exists("BaseFont")) {
            RefCountPtr<PDFObject> nameObj(pdfReader->QueryDictionaryObject(font.GetPtr(), "BaseFont"));
            ParsedPrimitiveHelper nameVal(nameObj.GetPtr());
            bool dimFound = false;
            std::string name(nameVal.ToString());
            std::regex regExp("-");
            std::string altName(std::regex_replace(name, regExp, "−"));  // for some reason dash is not a dash
            SFontDimensions standardDimensions;

            if (StandardFontsDimensions.count(name)) {
                standardDimensions = StandardFontsDimensions[name];
                dimFound = true;
            } else if(StandardFontsDimensions.count(altName)) {
                standardDimensions = StandardFontsDimensions[altName];
                dimFound = true;
            }

            if (dimFound) {
                descent = standardDimensions.descent;
                ascent = standardDimensions.ascent;

                for (auto& width: standardDimensions.widths) {
                    widths.insert(std::make_pair(width.first, width.second));
                }
            }
        }
    }

    if(!font->Exists("FontDescriptor")) {
        return;
    }

    // complete info with font descriptor
    PDFObjectCastPtr<PDFDictionary> fontDescriptor(pdfReader->QueryDictionaryObject(font.GetPtr(), "FontDescriptor"));

    RefCountPtr<PDFObject> descentObj(pdfReader->QueryDictionaryObject(fontDescriptor.GetPtr(), "Descent"));
    if (descentObj.GetPtr()) {
        ParsedPrimitiveHelper descentVal(descentObj.GetPtr());
        descent = descentVal.GetAsInteger();
    }

    RefCountPtr<PDFObject> ascentObj(pdfReader->QueryDictionaryObject(fontDescriptor.GetPtr(), "Ascent"));
    if (ascentObj.GetPtr()) {
        ParsedPrimitiveHelper ascentVal(ascentObj.GetPtr());
        ascent = ascentVal.GetAsInteger();
    }

    if (fontDescriptor->Exists("MissingWidth")) {
        RefCountPtr<PDFObject> defWidthObj(pdfReader->QueryDictionaryObject(fontDescriptor.GetPtr(), "MissingWidth"));
        ParsedPrimitiveHelper defWidthVal(defWidthObj.GetPtr());
        defaultWidth = defWidthVal.GetAsInteger();
    } else {
        defaultWidth = 0;
    }
}

void CFontDecoding::ParseCIDFontDimensions(PDFParser* pdfReader, PDFObjectCastPtr<PDFDictionary> font) {
    // get the descendents font
    PDFObjectCastPtr<PDFArray> descendentFonts(pdfReader->QueryDictionaryObject(font.GetPtr(), "DescendantFonts"));
    PDFObjectCastPtr<PDFDictionary> descendentFont(pdfReader->QueryArrayObject(descendentFonts.GetPtr(), 0));
    // default width is easily accessible directly via DW
    defaultWidth = 1000;
    if (descendentFont->Exists("DW")) {
        RefCountPtr<PDFObject> defaultWidthObj(pdfReader->QueryDictionaryObject(descendentFont.GetPtr(), "DW"));
        ParsedPrimitiveHelper defaultWidthVal(defaultWidthObj.GetPtr());
        defaultWidth = defaultWidthVal.GetAsInteger();
    }

    if (descendentFont->Exists("W")) {
        PDFObjectCastPtr<PDFArray> widthsArr(pdfReader->QueryDictionaryObject(descendentFont.GetPtr(), "W"));

        uint i = 0;
        while ( i < widthsArr->GetLength()) {

            RefCountPtr<PDFObject> widthObj(widthsArr->QueryObject(i));
            ParsedPrimitiveHelper widthVal(widthObj.GetPtr());

            signed int cFirst = widthVal.GetAsInteger();

            ++i;
            RefCountPtr<PDFObject> widthValNext(widthsArr->QueryObject(i));

            if (widthValNext->GetType() == PDFObject::ePDFObjectArray) {
                PDFObjectCastPtr<PDFArray> anArray(widthValNext.GetPtr());
                anArray->AddRef();

                ++i;
                // specified widths
                for (signed int j = 0; j < anArray->GetLength(); ++j) {
                    RefCountPtr<PDFObject> anArrayObj(anArray->QueryObject(j));
                    ParsedPrimitiveHelper anArrayVal(anArrayObj.GetPtr());
                    widths[cFirst + j] = anArrayVal.GetAsInteger();
                }

            } else {
                // same width for range
                if (widthsArr->GetLength() <= i) {
                   break;
                }
                RefCountPtr<PDFObject> cLastObj(widthsArr->QueryObject(i));
                ParsedPrimitiveHelper cLastVal(cLastObj.GetPtr());
                signed int cLast = cLastVal.GetAsInteger();
                ++i;

                if (widthsArr->GetLength() <= i) {
                   break;
                }
                RefCountPtr<PDFObject> cWidthObj(widthsArr->QueryObject(i));
                ParsedPrimitiveHelper cWidthVal(cWidthObj.GetPtr());
                signed int width = cWidthVal.GetAsInteger();
                ++i;

                for (signed int j = cFirst; j <= cLast; ++j) {
                    widths[j] = width;
                }
            }
        }
    }

    // complete info with font descriptor
    PDFObjectCastPtr<PDFDictionary> fontDescriptor(pdfReader->QueryDictionaryObject(descendentFont.GetPtr(), "FontDescriptor"));

    RefCountPtr<PDFObject> descentObj(pdfReader->QueryDictionaryObject(fontDescriptor.GetPtr(), "Descent"));
    ParsedPrimitiveHelper descentVal(descentObj.GetPtr());
    RefCountPtr<PDFObject> ascentObj(pdfReader->QueryDictionaryObject(fontDescriptor.GetPtr(), "Ascent"));
    ParsedPrimitiveHelper ascentVal(ascentObj.GetPtr());
    descent = descentVal.GetAsInteger();
    ascent = ascentVal.GetAsInteger();
}

void UnicodeToUTF8(unsigned int codepoint, std::string& str)
{
    if (codepoint <= 0x7f) {
        str.append(1, static_cast<char>(codepoint));
    } else if (codepoint <= 0x7ff) {
        str.append(1, static_cast<char>(0xc0 | ((codepoint >> 6) & 0x1f)));
        str.append(1, static_cast<char>(0x80 | (codepoint & 0x3f)));
    } else if (codepoint <= 0xffff) {
        str.append(1, static_cast<char>(0xe0 | ((codepoint >> 12) & 0x0f)));
        str.append(1, static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
        str.append(1, static_cast<char>(0x80 | (codepoint & 0x3f)));
    } else {
        str.append(1, static_cast<char>(0xf0 | ((codepoint >> 18) & 0x07)));
        str.append(1, static_cast<char>(0x80 | ((codepoint >> 12) & 0x3f)));
        str.append(1, static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
        str.append(1, static_cast<char>(0x80 | (codepoint & 0x3f)));
    }
}

void ToUnicodeEncoding(TUnicodeMap& toUnicodeMap, STextInput& textItem) {

    uint i = 0;
    while (i < textItem.asBytes.size()) {
        uint value = textItem.asBytes[i];
        i += 1;

        while (i < textItem.asBytes.size() && (toUnicodeMap.count(value) == 0)) {
            value = value * 256 + textItem.asBytes[i];
            i += 1;
        }

        if (toUnicodeMap.count(value)) {
            for (auto& mapItem: toUnicodeMap[value]) {
                UnicodeToUTF8(mapItem, textItem.asText);
            }
        }
    }
}

void ToSimpleEncoding(TEncodingMap* encodingMap, STextInput& textItem) {

    for (auto& encodedByte: textItem.asBytes) {
        if (encodingMap->count(encodedByte)) {
            std::string glyphName = encodingMap->at(encodedByte);
            std::vector<uint> mapping(AdobeGlyphList[glyphName]);

            for (auto& mapItem: mapping) {
                UnicodeToUTF8(mapItem, textItem.asText);
            }
        }
    }
}

void DefaultEncoding(STextInput& textItem) {
    for (auto& mapItem: textItem.asBytes) {
        UnicodeToUTF8(mapItem, textItem.asText);
    }
}

CFontDecoding::CFontDecoding(PDFParser* pdfReader, ObjectIDType fontRef) {

    PDFObjectCastPtr<PDFDictionary> font(pdfReader->ParseNewObject(fontRef));

    isSimpleFont = false;
    hasSimpleEncoding = false;
    hasToUnicode = false;
    ascent = 0;
    descent = 0;
    defaultWidth = 0;

    if(!font.GetPtr()) {
        return;
    }

    RefCountPtr<PDFObject> fontSubTypeObj(font->QueryDirectObject("Subtype"));
    ParsedPrimitiveHelper fontSubType(fontSubTypeObj.GetPtr());

    if (fontSubType.ToString() == "Type0") {
        isSimpleFont = false;
    } else {
        isSimpleFont = true;
    }

    // parse translating information
    if (font->Exists("ToUnicode")) {
        // to unicode map
        hasToUnicode = true;
        PDFObjectCastPtr<PDFIndirectObjectReference> unicodeObjRef(font->QueryDirectObject("ToUnicode"));
        ParseToUnicode(pdfReader, unicodeObjRef->mObjectID, toUnicodeMap);

    } else if (isSimpleFont) {
        // simple font encoding
        if (font->Exists("Encoding")) {
            RefCountPtr<PDFObject> fontEnc(font->QueryDirectObject("Encoding"));
            ParseSimpleFontEncoding(pdfReader, font, fontEnc);
        }
    }

    // parse dimensions information
    if (isSimpleFont) {
        ParseSimpleFontDimensions(pdfReader, font);
    }
    else {
        ParseCIDFontDimensions(pdfReader, font);
    }
}

CFontDecoding::CFontDecoding() {}

CFontDecoding::~CFontDecoding() {}

void CFontDecoding::Translate(STextInput& textInput) {
    if (hasToUnicode) {
        ToUnicodeEncoding(toUnicodeMap, textInput);
        textInput.translationMethod = TM_TO_UNICODE;
    } else if(hasSimpleEncoding) {
        ToSimpleEncoding(fromSimpleEncodingMap, textInput);
        textInput.translationMethod = TM_SIMPLE_ENCODING;
    } else {
        DefaultEncoding(textInput);
        textInput.translationMethod = TM_DEFAULT;
    }
}

double CFontDecoding::GetDecoderWidth(unsigned char code) {
    double width = defaultWidth;
    if (widths.count(code)) {
        width = widths[code];
    }

    return width;
}

void CFontDecoding::IterateTextDisplacements(std::vector<unsigned char>& encodedBytes, TCharsIterator CharsIterator) {
    if (isSimpleFont) {
        // one code per call
        for (unsigned char code: encodedBytes) {
            CharsIterator(GetDecoderWidth(code) / 1000, code);
        }
    } else if (hasToUnicode){
        // determine code per toUnicode (should be cmap, but i aint parsing it now, so toUnicode will do).
        // assuming horizontal writing mode
        std::size_t i = 0;
        while (i < encodedBytes.size()) {
            unsigned char code = encodedBytes[i];
            i += 1;

            while (i < encodedBytes.size() && (toUnicodeMap.count(code) == 0)) {
                code = code * 256 + encodedBytes[i];
                i += 1;
            }
            CharsIterator(GetDecoderWidth(code) / 1000, code);
        }
    } else {
        // default to 2 bytes. though i shuld be reading the cmap. and so also get the writing mode
        for (std::size_t i = 0; i < encodedBytes.size(); i += 2) {
            unsigned char code = encodedBytes[0] * 256 + encodedBytes[1];
            CharsIterator(GetDecoderWidth(code) / 1000, code);
        }
    }
}

