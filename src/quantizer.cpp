#include "quantizer.hpp"
#include <math.h>

std::pair<float, int> quantizeToScale(const float& noteInVoltage, const float& chordRootOffsetVoltage, const std::array<float, 12>& targetScale, const float& jazzAmount)
{
    float whole, fractional;
    float jazzThreshold = 1.0 - jazzAmount;
    whole = floor(noteInVoltage);
    fractional = noteInVoltage - whole;
    int nearestScaleToneIndex;
    float currentLowestNoteDistance = 1.0;
    for (int i = 0; i < 12; i++) {
        if ((float)targetScale[i] >= jazzThreshold) {
            float indexAsFloat = (float)i;
            float indexAsVoltage = (indexAsFloat/12) + chordRootOffsetVoltage;
            if (indexAsVoltage > 1.0) {
                indexAsVoltage = indexAsVoltage - 1;
            }
            float noteDistance = abs(indexAsVoltage - fractional);
            if (noteDistance < currentLowestNoteDistance) {
                currentLowestNoteDistance = noteDistance;
                nearestScaleToneIndex = i;
            }
        }
    }
    float baseNoteVoltage = (float)nearestScaleToneIndex / 12;
    float targetNoteVoltage = whole + baseNoteVoltage + chordRootOffsetVoltage;
    return std::make_pair(targetNoteVoltage, nearestScaleToneIndex);
}