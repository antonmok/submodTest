#include "pdfParser.h"
#include "PDFWriter.h"
#include "PDFParser.h"
#include "InputFile.h"
#include <iostream>

#include "placements-extraction.h"
#include "pdf-interpreter.h"
#include "transformations.h"
#include "font-decoding.h"

#include "MapIterator.h"
#include "ObjectsBasicTypes.h"
#include "PDFIndirectObjectReference.h"
#include "ParsedPrimitiveHelper.h"
#include "RetainMemory.h"

using namespace PDFHummus;

void Tc(double charSpace, CollectionState& state) {
    state.CurrentTextState().charSpace = charSpace;
}

void Tw(double wordSpace, CollectionState& state) {
    state.CurrentTextState().wordSpace = wordSpace;
}

void setTm(double newM[6], CollectionState& state) {

    STextState& currentTextEnv = state.CurrentTextState();

    std::copy(newM, newM + 6, currentTextEnv.tlm);
    std::copy(newM, newM + 6, currentTextEnv.tm);

    currentTextEnv.tmDirty = true;
    currentTextEnv.tlmDirty = true;
}

void Td(double tx, double ty, CollectionState& state) {

    double MTmp[6];
    double M1[6] = {1, 0, 0, 1, tx, ty};

    MultiplyMatrix(M1, state.CurrentTextState().tlm, MTmp);
    setTm(MTmp, state);
}

void TL(double leading, CollectionState& state) {
    state.CurrentTextState().leading = leading;
}

void TStar(CollectionState& state) {
    // there's an error in the book explanation
    // but we know better. leading goes below,
    // not up. this is further explicated by
    // the TD explanation
    Td(0, -state.CurrentTextState().leading, state);
}

void textPlacement(std::vector<STextInput>& input, CollectionState& state) {

    SText item;

    item.text = input;

    SGraphicState& currentGraphState = state.CurrentGraphicState();
    std::copy(std::begin(currentGraphState.ctm), std::end(currentGraphState.ctm), std::begin(item.ctm));

    STextState txtState;
    state.CloneCurrentTextState(txtState);

    item.textState = txtState;

    state.CurrentTextState().tmDirty = false;
    state.CurrentTextState().tlmDirty = false;
    state.texts.push_back(item);
}

void Quote(std::vector<STextInput>& text, CollectionState& state) {
    TStar(state);
    textPlacement(text, state);
}

void PrintOperandInfo(std::string opName, PDFObject::EPDFObjectType opType) {

    std::string typeStr;

    switch (opType) {
    case PDFObject::ePDFObjectArray:
        typeStr = "Array";
        break;
    case PDFObject::ePDFObjectInteger:
        typeStr = "Integer";
        break;
    case PDFObject::ePDFObjectReal:
        typeStr = "Real";
        break;
    case PDFObject::ePDFObjectLiteralString:
        typeStr = "LiteralString";
        break;
    case PDFObject::ePDFObjectSymbol:
        typeStr = "Symbol";
        break;
    case PDFObject::ePDFObjectHexString:
        typeStr = "hex string";
        break;
    case PDFObject::ePDFObjectBoolean:
        typeStr = "bool";
        break;
    case PDFObject::ePDFObjectName:
        typeStr = "Name";
        break;
    default:
        typeStr = "Unknown";
        break;
    }

    std::cout << opName << std::endl;
    std::cout << typeStr << std::endl;
    std::cout << "----------------------------" << std::endl;

}

TOperatorHandler CollectPlacements(ResourceData& resources, TPlacements& placements/*, TFormsUsed& formsUsed*/) {

    std::shared_ptr<CollectionState> state = std::make_shared<CollectionState>();

    return [state, &resources, &placements/*, &formsUsed*/](const std::string& operatorName, std::shared_ptr<TOperandsStack> operands) {

        // Graphic State Operators
        if (operatorName == "q") {
            state->PushGraphicState();
            return;
        }

        if (operatorName == "Q") {
            state->PopGraphicState();
            return;
        }

        // Text elements operators
        if (operatorName == "BT") {
            state->StartTextElement();
            return;
        }

        if (operatorName == "ET") {
            state->EndTextElement(placements);
            return;
        }

        if (operatorName == "Tm") { // Real AND Integer
            if (operands.get()->size() < 6) {
                std::cerr << "not enough values for setTm" << std::endl;
                return;
            }
            double MTmp[6];
            for (std::size_t i = 0; i < 6; ++i) {
                ParsedPrimitiveHelper opValue(operands.get()->at(i).GetPtr());
                MTmp[i] = opValue.GetAsDouble();
            }
            setTm(MTmp, *(state.get()));
            return;
        }

        if (operatorName == "Tf") { // 0: Name 1: Real AND Integer
            if (!operands.get()->size()) return;
            ParsedPrimitiveHelper opValue0(operands.get()->at(0).GetPtr());
            ParsedPrimitiveHelper opValue1(operands.get()->at(1).GetPtr());

            std::map<std::string, ObjectIDType>::iterator itFont = resources.fonts.find(opValue0.ToString());

            if (itFont != resources.fonts.end()) {
                state->CurrentTextState().font.fontRef = itFont->second;
                state->CurrentTextState().font.size = opValue1.GetAsDouble();
            }
            return;
        }

        if (operatorName == "Tj") { // LiteralString AND HEXString
            if (!operands.get()->size()) return;
            STextInput txtInput;
            std::vector<STextInput> txtInputArr;
            ParsedPrimitiveHelper opValue(operands.get()->at(0).GetPtr());

            txtInput.asEncodedText.assign(opValue.ToString());
            for(std::string::size_type i = 0; i < txtInput.asEncodedText.size(); ++i) {
                txtInput.asBytes.push_back((unsigned char)txtInput.asEncodedText.at(i));
            }
            txtInputArr.push_back(txtInput);
            textPlacement(txtInputArr, *(state.get()));
            return;
        }

        if (operatorName == "cm") { // Integers
            double newMatrix[6];
            double tmpMatrix[6];

            if (operands.get()->size() < 6) {
                std::cerr << "not enough values for ctm matrix" << std::endl;
                return;
            }

            for (std::size_t i = 0; i < 6; ++i) {
                ParsedPrimitiveHelper opValue(operands.get()->at(i).GetPtr());
                newMatrix[i] = opValue.GetAsDouble();
            }

            MultiplyMatrix(newMatrix, state->CurrentGraphicState().ctm, tmpMatrix);
            std::copy(std::begin(tmpMatrix), std::end(tmpMatrix), std::begin(state->CurrentGraphicState().ctm));

            return;
        }

        if (operatorName == "gs") { // Name
            if (!operands.get()->size()) return;

            ParsedPrimitiveHelper opValue(operands.get()->at(0).GetPtr());
            std::map<std::string, ExtGStateItem>::iterator itExtGState = resources.extGStates.find(opValue.ToString());

            if (itExtGState != resources.extGStates.end()) {
                if (itExtGState->second.fontRef) {
                    state->CurrentTextState().font.fontRef = itExtGState->second.fontRef;
                    state->CurrentTextState().font.size = itExtGState->second.size;
                }
            }
            return;
        }

        if (operatorName == "Tc") { // Real AND Integer
            if (!operands.get()->size()) return;
            ParsedPrimitiveHelper opValue(operands.get()->at(0).GetPtr());
            Tc(opValue.GetAsDouble(), *(state.get()));
            return;
        }

        if (operatorName == "Tw") { // Real
            if (!operands.get()->size()) return;
            ParsedPrimitiveHelper opValue(operands.get()->at(0).GetPtr());
            Tw(opValue.GetAsDouble(), *(state.get()));
            return;
        }

        if (operatorName == "TJ") { // Array
            if (!operands.get()->size()) return;

            PDFObjectCastPtr<PDFArray> textArr(operands.get()->at(0).GetPtr());
            unsigned long len = textArr->GetLength();
            std::vector<STextInput> txtInputArr;
            STextInput txtInput;

            for (unsigned long i = 0; i < len; ++i) {
                RefCountPtr<PDFObject> obj(textArr->QueryObject(i));
                ParsedPrimitiveHelper opValue(obj.GetPtr());

                if (obj->GetType() == PDFObject::ePDFObjectHexString || obj->GetType() == PDFObject::ePDFObjectLiteralString) { // string first

                    txtInput.tx = 0;
                    txtInput.asEncodedText.assign(opValue.ToString());

                    txtInput.useTx = false;

                    for (std::string::size_type i = 0; i < txtInput.asEncodedText.size(); ++i) {
                        txtInput.asBytes.push_back((unsigned char)txtInput.asEncodedText.at(i));
                    }

                    txtInputArr.push_back(txtInput);
                    txtInput.asEncodedText.clear();
                    txtInput.asBytes.clear();

                } else { // displacement second
                    txtInput.tx = opValue.GetAsDouble();
                    txtInput.useTx = true;
                    txtInputArr.push_back(txtInput);
                }
            }

            textPlacement(txtInputArr, *(state.get()));
            return;
        }

        // TODO: make forms parsing per-page based
        // XObject placement
        if (operatorName == "Do") { // Name
            /*if (!operands.get()->size()) return;

            ParsedPrimitiveHelper opValue(operands.get()->at(0).GetPtr());

            // add placement, if form, and mark for later inspection
            std::map<std::string, FormItem>::iterator itForms = resources.forms.find(opValue.ToString());
            if (itForms != resources.forms.end()) {

                FormItem form(resources.forms[opValue.ToString()]);
                Placement placement;

                placement.type.assign("xobject");
                placement.objectId = form.id;
                std::copy(std::begin(form.matrix), std::end(form.matrix), std::begin(placement.matrix));
                std::copy(std::begin(state->CurrentGraphicState().ctm), std::end(state->CurrentGraphicState().ctm), std::begin(placement.ctm));

                placements.push_back(placement);

                // add for later inspection (helping the extraction method a bit..[can i factor out? interesting enough?])
                formsUsed[itForms->second.id] = itForms->second.xobject;
            }*/
            return;
        }

        if (operatorName == "Tz") { // Real m.b.
            if (!operands.get()->size()) return;
            ParsedPrimitiveHelper opValue(operands.get()->at(0).GetPtr());
            state->CurrentTextState().scale = opValue.GetAsDouble();
            return;
        }

        if (operatorName == "TL") { // Real m.b.
            if (!operands.get()->size()) return;
            ParsedPrimitiveHelper opValue(operands.get()->at(0).GetPtr());
            TL(opValue.GetAsDouble(), *(state.get()));
            return;
        }

        if (operatorName == "Ts") { // Real m.b.
            if (!operands.get()->size()) return;
            ParsedPrimitiveHelper opValue(operands.get()->at(0).GetPtr());
            state->CurrentTextState().rise = opValue.GetAsDouble();
            return;
        }

        // Text positioning operators
        if (operatorName == "Td") { // 0: Real 1: Real AND Integer
            if (!operands.get()->size()) return;
            ParsedPrimitiveHelper opValue0(operands.get()->at(0).GetPtr());
            ParsedPrimitiveHelper opValue1(operands.get()->at(1).GetPtr());
            Td(opValue0.GetAsDouble(), opValue1.GetAsDouble(), *(state.get()));
            return;
        }

        if (operatorName == "TD") { // 0: Integer 1: Real
            if (!operands.get()->size()) return;
            ParsedPrimitiveHelper opValue0(operands.get()->at(0).GetPtr());
            ParsedPrimitiveHelper opValue1(operands.get()->at(1).GetPtr());
            TL(-opValue1.GetAsDouble(), *(state.get()));
            Td(opValue0.GetAsDouble(), opValue1.GetAsDouble(), *(state.get()));
            return;
        }

        if (operatorName == "T*") {
            TStar(*(state.get()));
            return;
        }

        if (operatorName == "\'") { // string
            if (!operands.get()->size()) return;
            STextInput txtInput;
            std::vector<STextInput> txtInputArr;
            ParsedPrimitiveHelper opValue(operands.get()->at(0).GetPtr());
            txtInput.asEncodedText.assign(opValue.ToString());
            for(std::string::size_type i = 0; i < txtInput.asEncodedText.size(); ++i) {
                txtInput.asBytes.push_back((unsigned char)txtInput.asEncodedText.at(i));
            }
            txtInputArr.push_back(txtInput);
            Quote(txtInputArr, *(state.get()));
            return;
        }

        if (operatorName == "\"") { // 0: Real 1: Real AND Integer 2:
            if (operands.get()->size() < 3) {
                std::cerr << "not enough values for Quote" << std::endl;
                return;
            }
            ParsedPrimitiveHelper opValue0(operands.get()->at(0).GetPtr());
            ParsedPrimitiveHelper opValue1(operands.get()->at(1).GetPtr());
            ParsedPrimitiveHelper opValue2(operands.get()->at(2).GetPtr());

            STextInput txtInput;
            txtInput.asEncodedText.assign(opValue2.ToString());
            for(std::string::size_type i = 0; i < txtInput.asEncodedText.size(); ++i) {
                txtInput.asBytes.push_back((unsigned char)txtInput.asEncodedText.at(i));
            }
            std::vector<STextInput> txtInputArr;
            txtInputArr.push_back(txtInput);

            Tw(opValue0.GetAsDouble(), *(state.get()));
            Tc(opValue1.GetAsDouble(), *(state.get()));
            Quote(txtInputArr, *(state.get()));
            return;
        }
    };
}

void IterateDict(RefCountPtr<PDFDictionary> dict)
{
    if (dict.GetPtr()) {

        RefCountPtr<PDFName> key;
        MapIterator<PDFNameToPDFObjectMap> it = dict->GetIterator();

        while(it.MoveNext())
        {
            key = it.GetKey();

            std::cerr<< key->GetValue() << std::endl;
        }
    }
}

void ReadResources(std::shared_ptr<TDictionaries> resourcesDicts, PDFParser* pdfReader, ResourceData& result) {

    if (MultiDictHelperExists("ExtGState", resourcesDicts.get())) {

        PDFObjectCastPtr<PDFDictionary> extGStatesEntry(MultiDictHelperQueryDictionaryObject("ExtGState", pdfReader, resourcesDicts.get()).GetPtr());

        if (extGStatesEntry.GetPtr()) {
            extGStatesEntry->AddRef();

            MapIterator<PDFNameToPDFObjectMap> it(extGStatesEntry.GetPtr()->GetIterator());
            RefCountPtr<PDFName> key;
            PDFObjectCastPtr<PDFIndirectObjectReference> value;

            while (it.MoveNext()) {

                key = it.GetKey();
                value = it.GetValue();

                PDFObjectCastPtr<PDFDictionary> extGState;

                if (value->GetType() == PDFObject::ePDFObjectIndirectObjectReference) {
                    PDFObjectCastPtr<PDFDictionary> extGStateTmpRef(pdfReader->ParseNewObject(value->mObjectID));
                    extGState = extGStateTmpRef;
                } else {
                    PDFObjectCastPtr<PDFDictionary> extGStateTmpRef(value.GetPtr());
                    extGState = extGStateTmpRef;
                }

                if(extGState.GetPtr()) {

                    ExtGStateItem item;

                    item.theObject = extGState.GetPtr();

                    //IterateDict(extGState);

                    if (extGState->Exists("Font")) {
                        PDFObjectCastPtr<PDFArray> fontEntry = pdfReader->QueryDictionaryObject(extGState.GetPtr(), "Font");

                        if (fontEntry.GetPtr()) {
                            // TODO: determine objects type
                            item.fontRef = 0;
                            item.size = 0;
                            RefCountPtr<PDFObject> obj0(fontEntry.GetPtr()->QueryObject(0));
                            RefCountPtr<PDFObject> obj1(fontEntry.GetPtr()->QueryObject(1));

                            PDFObject::EPDFObjectType type0 = obj0.GetPtr()->GetType();
                            PDFObject::EPDFObjectType type1 = obj1.GetPtr()->GetType();

                            std::cerr << "extGState font found: " << type0 << " " << type1 << std::endl;
                        }
                    }

                    result.extGStates.insert(std::make_pair(key->GetValue(), item));
                }

            }
        }
    }

    if (MultiDictHelperExists("Font", resourcesDicts.get())) {

        PDFObjectCastPtr<PDFDictionary> fontsEntry(MultiDictHelperQueryDictionaryObject("Font", pdfReader, resourcesDicts.get()).GetPtr());

        if (fontsEntry.GetPtr()) {

            fontsEntry.GetPtr()->AddRef();

            MapIterator<PDFNameToPDFObjectMap> it(fontsEntry->GetIterator());

            while (it.MoveNext()) {
                result.fonts.insert(std::make_pair(it.GetKey()->GetValue(), ((PDFIndirectObjectReference*)it.GetValue())->mObjectID));
            }
        }
    }
}

CFontDecoding* FetchFontDecoder(SText& item, PDFParser* pdfReader, TFontDecoders& decoders) {

    if(!decoders.count(item.textState.font.fontRef)) {
        CFontDecoding fontDecoder(pdfReader, item.textState.font.fontRef);
        decoders.insert(std::make_pair(item.textState.font.fontRef, fontDecoder));
    }

    return &decoders[item.textState.font.fontRef];
}

void TranslateText(PDFParser* pdfReader, STextInput& textItem, TFontDecoders& decoders, SText& item) {
    CFontDecoding* decoder = FetchFontDecoder(item, pdfReader, decoders);
    decoder->Translate(textItem);
}

void TranslatePlacements(TFontDecoders& decoders, PDFParser* pdfReader, TPlacements& placements) {
    // iterate the placements, getting the texts and translating them
    for (auto& placement: placements) {

        if (placement.type == "text") {

            for (auto& item: placement.text) {
                // save all text
                for (auto& textItem: item.text) {
                    if(textItem.asBytes.size()) {
                        for (auto& txtByte: textItem.asBytes) {
                            item.allText.asBytes.push_back(txtByte);
                        }
                    }
                }

                TranslateText(pdfReader, item.allText, decoders, item);
            }
        }
    }
}

void ComputePlacementsDimensions(TFontDecoders& decoders, PDFParser* pdfReader, TPlacements& placements) {
    // iterate the placements computing bounding boxes
    for (Placement& placement: placements) {

        if (placement.type == "text") {
            // this is a BT..ET sequance
            double nextPlacementDefaultTm[6] = {};
            bool nextPlacementDefaultTmNotEmpty = false;

            for (SText& item: placement.text) {
                // if matrix is not dirty (no matrix changing operators were running betwee items), replace with computed matrix of the previous round.
                if(!item.textState.tmDirty && nextPlacementDefaultTmNotEmpty) {
                    std::copy(std::begin(nextPlacementDefaultTm), std::end(nextPlacementDefaultTm), std::begin(item.textState.tm));
                }

                // Compute matrix and placement after this text
                CFontDecoding* decoder = FetchFontDecoder(item, pdfReader, decoders);

                double accumulatedDisplacement = 0;
                double minPlacement = 0;
                double maxPlacement = 0;

                std::copy(std::begin(item.textState.tm), std::end(item.textState.tm), std::begin(nextPlacementDefaultTm));
                nextPlacementDefaultTmNotEmpty = true;

                for (STextInput& textItem: item.text) {

                    if (!textItem.useTx) {
                         // marks a string
                        decoder->IterateTextDisplacements(textItem.asBytes, [&item, &nextPlacementDefaultTm, &accumulatedDisplacement, &minPlacement, &maxPlacement](double displacement, unsigned char charCode) {
                            double tx = (displacement * item.textState.font.size + item.textState.charSpace + (charCode == 32 ? item.textState.wordSpace : 0)) * item.textState.scale / 100;
                            accumulatedDisplacement += tx;

                            if (accumulatedDisplacement < minPlacement) {
                                minPlacement = accumulatedDisplacement;
                            }
                            if (accumulatedDisplacement > maxPlacement) {
                                maxPlacement = accumulatedDisplacement;
                            }

                            double newMatrix[6] = {1,0,0,1,tx,0};
                            double tmpMatrix[6] = {};

                            MultiplyMatrix(newMatrix, nextPlacementDefaultTm, tmpMatrix);
                            std::copy(std::begin(tmpMatrix), std::end(tmpMatrix), std::begin(nextPlacementDefaultTm));
                        });
                    } else {
                        // displacement info, use it
                        double tx = ((-textItem.tx / 1000) * item.textState.font.size) * item.textState.scale / 100;
                        accumulatedDisplacement += tx;

                        if (accumulatedDisplacement < minPlacement) {
                            minPlacement = accumulatedDisplacement;
                        }
                        if (accumulatedDisplacement > maxPlacement) {
                            maxPlacement = accumulatedDisplacement;
                        }

                        double newMatrix[6] = {1,0,0,1,tx,0};
                        double tmpMatrix[6] = {};

                        MultiplyMatrix(newMatrix, nextPlacementDefaultTm, tmpMatrix);
                        std::copy(std::begin(tmpMatrix), std::end(tmpMatrix), std::begin(nextPlacementDefaultTm));
                    }
                }

                double descentPlacement = (decoder->descent + item.textState.rise) * item.textState.font.size / 1000;
                double ascentPlacement = (decoder->ascent + item.textState.rise) * item.textState.font.size / 1000;
                item.localBBox[0] = minPlacement;
                item.localBBox[1] = descentPlacement;
                item.localBBox[2] = maxPlacement;
                item.localBBox[3] = ascentPlacement;
            }
        }
    }
}

void FlattenPlacements(TPlacements& placements, ParsedPDFDataListPerPage& parsedDataListPerPg) {

    ParsedPDFDataList parsedDataList;
    parsedDataListPerPg.push_back(parsedDataList);

    for (Placement& pagePlacement: placements) {

        for (SText& textPlacement: pagePlacement.text) {

            ParsedPDFData parsedData;
            double matrix[6];

            MultiplyMatrix(textPlacement.textState.tm, textPlacement.ctm, matrix);
            std::copy(std::begin(matrix), std::end(matrix), std::begin(parsedData.matrix));

            parsedData.text = textPlacement.allText.asText;

            std::copy(std::begin(textPlacement.localBBox), std::end(textPlacement.localBBox), std::begin(parsedData.localBBox));
            TransformBox(textPlacement.localBBox, parsedData.matrix, parsedData.globalBBox);

            parsedDataListPerPg.back().push_back(parsedData);
        }
        if (parsedDataListPerPg.back().size()) {
            parsedDataListPerPg.back().back().lastItemInBlock = true;
        }
    }
}

int ValidatePDF(const std::string& filePath) {
    InputFile pdfFile;
    PDFParser parser;

    if (EStatusCode::eFailure == pdfFile.OpenFile(filePath)) {
        std::cerr << "File not found" << std::endl;
        return 2;
    }

    EStatusCode status = parser.StartPDFParsing(pdfFile.GetInputStream());
    pdfFile.CloseFile();
    parser.ResetParser();

    if (status == EStatusCode::eFailure) {
        std::cerr << "PDFWriter failed to parse pdf" << std::endl;
        return 4;
    }

    if (status == EStatusCode::eUnsuportedEncMethod) {
        std::cerr << "Pdf is protected by password" << std::endl;
        return 5;
    }

    return 0;
}

unsigned int ParsePDF(const std::string& filePath, ParsedPDFDataListPerPage& parsedDataListPerPg, int& errCode) {

    InputFile pdfFile;
    PDFParser parser;

    if (EStatusCode::eFailure == pdfFile.OpenFile(filePath)) {
        std::cerr << "File not found" << std::endl;
        errCode = 2;
        return 0;
    }

    EStatusCode status = parser.StartPDFParsing(pdfFile.GetInputStream());

    if (status == EStatusCode::eFailure) {
        std::cerr << "PDFWriter failed to parse pdf" << std::endl;
        errCode = 4;
        return 0;
    }

    if (status == EStatusCode::eUnsuportedEncMethod) {
        std::cerr << "Pdf is protected by password" << std::endl;
        errCode = 5;
        return 0;
    }

    TFontDecoders fontDecoders;
    unsigned int pgCnt = parser.GetPagesCount();

    for (unsigned int i = 0; i < pgCnt; ++i) {
        TPlacements placements;
        // extract placements
        InspectPage(&parser, CollectPlacements, ReadResources, placements, i);
        // decode placements bytes to text
        TranslatePlacements(fontDecoders, &parser, placements);
        // compute placements matrixes
        ComputePlacementsDimensions(fontDecoders, &parser, placements);
        // construct parsed PDF data object
        FlattenPlacements(placements, parsedDataListPerPg);
    }

    parser.ResetParser();
    pdfFile.CloseFile();
    RetainMemory::Instance()->Release();

    return pgCnt;
}
