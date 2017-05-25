#include "collection-state.h"


void CloneTextEnv(STextState& stateIn, STextState& stateOut) {

    stateOut.charSpace = stateIn.charSpace;
    stateOut.wordSpace = stateIn.wordSpace;
    stateOut.scale = stateIn.scale;
    stateOut.leading = stateIn.leading;
    stateOut.rise = stateIn.rise;

    stateOut.font.fontRef = stateIn.font.fontRef;
    stateOut.font.size = stateIn.font.size;

    std::copy(std::begin(stateIn.tm), std::end(stateIn.tm), std::begin(stateOut.tm));
    std::copy(std::begin(stateIn.tlm), std::end(stateIn.tlm), std::begin(stateOut.tlm));

    stateOut.tmDirty = stateIn.tmDirty;
    stateOut.tlmDirty = stateIn.tlmDirty;
}

void CloneGraphicEnv(SGraphicState& stateIn, SGraphicState& stateOut) {

    std::copy(std::begin(stateIn.ctm), std::end(stateIn.ctm), std::begin(stateOut.ctm));

    CloneTextEnv(stateIn.text, stateOut.text);
}

CollectionState::CollectionState() {

    SGraphicState graphState;
    graphicStateStack.push_back(graphState);
    inTextElement = false;
}

void CollectionState::PushGraphicState() {

    SGraphicState graphState;
    CloneGraphicEnv(graphicStateStack.at(graphicStateStack.size() - 1), graphState);
    graphicStateStack.push_back(graphState);

    if (inTextElement) {
        STextState textState;
        CloneTextEnv(textElementTextStack.at(textElementTextStack.size() - 1), textState);
        textElementTextStack.push_back(textState);
    }
}

void CollectionState::PopGraphicState() {
    if (graphicStateStack.size() > 1) {
        graphicStateStack.pop_back();
    }

    if (inTextElement && textElementTextStack.size() > 1) {
        textElementTextStack.pop_back();
    }
}


SGraphicState& CollectionState::CurrentGraphicState() {
    return graphicStateStack.at(graphicStateStack.size() - 1);
}

STextState& CollectionState::CurrentTextState() {
    if (inTextElement) {
        return textElementTextStack.at(textElementTextStack.size() - 1);
    } else {
        return graphicStateStack.at(graphicStateStack.size() - 1).text;
    }
}

void CollectionState::CloneCurrentTextState(STextState& stateOut) {
    CloneTextEnv(CurrentTextState(), stateOut);
}

void CollectionState::StartTextElement() {
    inTextElement = true;

    STextState txtState;
    CloneTextEnv(CurrentGraphicState().text, txtState);
    textElementTextStack.push_back(txtState);
    texts.clear();
}

void CollectionState::EndTextElement(TPlacements& placements) {

    // save text properties to persist after gone (some of them...)
    STextState latestTextState;
    CloneCurrentTextState(latestTextState);

    inTextElement = false;
    textElementTextStack.clear();

    Placement placement;

    placement.type.assign("text");
    placement.text = texts;

    placements.push_back(placement);

    texts.clear();

    // copy persisted data to top text state
    STextState& persistingTextState = CurrentTextState();

    persistingTextState.charSpace = latestTextState.charSpace;
    persistingTextState.wordSpace = latestTextState.wordSpace;
    persistingTextState.scale = latestTextState.scale;
    persistingTextState.leading = latestTextState.leading;
    persistingTextState.rise = latestTextState.rise;
    persistingTextState.font = latestTextState.font;
}

