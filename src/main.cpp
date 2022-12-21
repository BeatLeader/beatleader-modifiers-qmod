#include "main.hpp"
#include "ModConfig.hpp"

#include "include/CharacteristicsManager.hpp"
#include "include/RthythmGameModifier.hpp"

#include "config-utils/shared/config-utils.hpp"

#include "GlobalNamespace/NoteController.hpp"
#include "GlobalNamespace/GameNoteController.hpp"
#include "GlobalNamespace/BurstSliderGameNoteController.hpp"
#include "GlobalNamespace/BombNoteController.hpp"
#include "GlobalNamespace/BoxCuttableBySaber.hpp"
#include "GlobalNamespace/CuttableBySaber.hpp"
#include "GlobalNamespace/AppInit.hpp"

#include "HMUI/Touchable.hpp"

#include "questui/shared/CustomTypes/Components/MainThreadScheduler.hpp"
#include "questui/shared/QuestUI.hpp"

#include "beatsaber-hook/shared/config/rapidjson-utils.hpp"
#include "beatsaber-hook/shared/rapidjson/include/rapidjson/filereadstream.h"

#include "UnityEngine/GameObject.hpp"

using namespace GlobalNamespace;
using namespace QuestUI;
using namespace UnityEngine;

ModInfo modInfo; // Stores the ID and version of our mod, and is sent to the modloader upon startup

DEFINE_CONFIG(ModConfig);

// Returns a logger, useful for printing debug messages
Logger& getLogger() {
    static Logger* logger = new Logger(modInfo, LoggerOptions(false, true));
    return *logger;
}

// Called at the early stages of game loading
extern "C" void setup(ModInfo& info) {
    info.id = ID;
    info.version = VERSION;
    modInfo = info;
	
    getModConfig().Init(modInfo); // Load the config file
    getLogger().info("Completed setup!");
}

MAKE_HOOK_MATCH(AppInitStart, &AppInit::Start, void,
    AppInit *self) {
     BeatLeaderModifiers::InstallCharacteristics();
     BeatLeaderModifiers::InstallRthythmGame();

     AppInitStart(self);
}

// Called later on in the game loading - a good time to install function hooks
extern "C" void load() {
    il2cpp_functions::Init();

    LoggerContextObject logger = getLogger().WithContext("load");

    QuestUI::Init();

    getLogger().info("Installing main hooks...");
    
    INSTALL_HOOK(logger, AppInitStart);

    getLogger().info("Installed main hooks!");
}