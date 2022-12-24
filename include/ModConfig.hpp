#pragma once
#include "config-utils/shared/config-utils.hpp"
#include "bs-utils/shared/utils.hpp"

DECLARE_CONFIG(ModConfig,

    CONFIG_VALUE(QubeSize, float, "Cube size", 0.85, "Yes");

    CONFIG_INIT_FUNCTION(
        CONFIG_INIT_VALUE(QubeSize);
    )
)

inline bool UploadDisabledByReplay() {
    for (auto kv : bs_utils::Submission::getDisablingMods()) {
        if (kv.id == "Replay") {
            return true;
        }
    } 
    return false;
}