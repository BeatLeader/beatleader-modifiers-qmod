#pragma once

namespace BeatLeaderModifiers {
    enum CustomCharacterisitic {
        standard = 0,
        betterScoring = 1
    };

    extern CustomCharacterisitic customCharacterisitic;

   void InstallCharacteristics();
}