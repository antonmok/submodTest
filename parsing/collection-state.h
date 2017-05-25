#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include "ObjectsBasicTypes.h"

struct TFont {
    TFont() {
        fontRef = 0;
        size = 0;
    }
    ObjectIDType fontRef;
    unsigned long size;
};

struct STextState {

    STextState () {
        charSpace = 0;
        wordSpace = 0;
        scale = 100;
        leading = 0;
        rise = 0;

        tm[0] = 1;
        tm[1] = 0;
        tm[2] = 0;
        tm[3] = 1;
        tm[4] = 0;
        tm[5] = 0;

        tlm[0] = 1;
        tlm[1] = 0;
        tlm[2] = 0;
        tlm[3] = 1;
        tlm[4] = 0;
        tlm[5] = 0;

        tmDirty = true;
        tlmDirty = true;
    }

    double charSpace;
    double wordSpace;
    double scale;
    double leading;
    double rise;
    TFont font;
    double tm[6];
    double tlm[6];
    bool tmDirty;
    bool tlmDirty;
};

struct SGraphicState {

    SGraphicState() {
        ctm[0] = 1;
        ctm[1] = 0;
        ctm[2] = 0;
        ctm[3] = 1;
        ctm[4] = 0;
        ctm[5] = 0;
    }

    double ctm[6];
    STextState text;
};

enum TranslationMethod {
    TM_TO_UNICODE = 0,
    TM_SIMPLE_ENCODING,
    TM_DEFAULT,
};

struct STextInput {

    STextInput() {
        useTx = false;
    };

    std::string asEncodedText;
    std::string asText;
    std::vector<unsigned char> asBytes;
    TranslationMethod translationMethod;
    double tx;
    bool useTx;
};

struct SText {
    std::vector<STextInput> text;
    STextInput allText;
    double ctm[6];
    double localBBox[4];
    STextState textState;
};

struct Placement {
    std::string type;
    ObjectIDType objectId;
    double matrix[6];
    double ctm[6];
    std::vector<SText> text;
};

typedef std::vector<Placement> TPlacements;

class CollectionState {

public:

    CollectionState();

    void PushGraphicState();
    void PopGraphicState();

    SGraphicState& CurrentGraphicState();
    STextState& CurrentTextState();
    void CloneCurrentTextState(STextState& stateOut);

    void StartTextElement();
    void EndTextElement(TPlacements& placements);

    std::vector<SText> texts;

private:

    bool inTextElement;
    std::vector<SGraphicState> graphicStateStack;
    std::vector<STextState> textElementTextStack;
};
