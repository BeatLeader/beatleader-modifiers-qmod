#include "include/CharacteristicsManager.hpp"

#include "include/Sprites.hpp"

#include "UnityEngine/Vector2.hpp"
#include "UnityEngine/Sprite.hpp"
#include "UnityEngine/ScriptableObject.hpp"
#include "UnityEngine/UI/VertexHelper.hpp"

#include "GlobalNamespace/BeatmapCharacteristicCollectionSO.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"
#include "GlobalNamespace/StandardLevelDetailView.hpp"
#include "GlobalNamespace/CustomDifficultyBeatmap.hpp"
#include "GlobalNamespace/IBeatmapLevel.hpp"
#include "GlobalNamespace/IBeatmapLevelData.hpp"
#include "GlobalNamespace/IPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/IDifficultyBeatmapSet.hpp"
#include "GlobalNamespace/IDifficultyBeatmap.hpp"
#include "GlobalNamespace/IBeatmapDataBasicInfo.hpp"
#include "GlobalNamespace/BeatmapDifficulty.hpp"
#include "GlobalNamespace/PlayerData.hpp"
#include "GlobalNamespace/CustomDifficultyBeatmapSet.hpp"
#include "GlobalNamespace/BeatmapLevelData.hpp"

#include "GlobalNamespace/StandardLevelScenesTransitionSetupDataSO.hpp"
#include "GlobalNamespace/OverrideEnvironmentSettings.hpp"
#include "GlobalNamespace/ColorScheme.hpp"
#include "GlobalNamespace/GameplayModifiers.hpp"
#include "GlobalNamespace/PlayerSpecificSettings.hpp"
#include "GlobalNamespace/PracticeSettings.hpp"

#include "BeatmapSaveDataVersion3/BeatmapSaveData.hpp"

#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "beatsaber-hook/shared/utils/logging.hpp"

#include "System/Threading/Tasks/Task_1.hpp"

#include "main.hpp"

#include <vector>

using namespace std;
using namespace UnityEngine;
using namespace GlobalNamespace;

template <class T>
    constexpr ArrayW<T> listToArrayW(::System::Collections::Generic::IReadOnlyList_1<T>* list) {
        return ArrayW<T>(reinterpret_cast<Array<T>*>(list));
    }

namespace BeatLeaderModifiers {
    // Future NSGolova, make it array, I'm lazy, sorry
    static BeatmapCharacteristicSO* customCharacteristics;

    CustomCharacterisitic customCharacterisitic = CustomCharacterisitic::standard;

    static ArrayW<CustomDifficultyBeatmap*> CreateCustomDifficulties(
            ArrayW<IDifficultyBeatmap *> difficultyBeatmaps,
            CustomDifficultyBeatmapSet* beatmapSet
    ) {
        ArrayW<CustomDifficultyBeatmap*> customDifficulties(difficultyBeatmaps.Length());

        for (int i = 0; i < difficultyBeatmaps.Length(); i++) {
            IDifficultyBeatmap* difficultyBeatmap = difficultyBeatmaps[i];
            IBeatmapDataBasicInfo* beatmapDataBasicInfo = difficultyBeatmap->GetBeatmapDataBasicInfoAsync()->get_Result();
            
            CustomDifficultyBeatmap* customBeatmap = CustomDifficultyBeatmap::New_ctor(
                difficultyBeatmap->get_level(),
                reinterpret_cast<IDifficultyBeatmapSet*>(beatmapSet),
                difficultyBeatmap->get_difficulty(),
                difficultyBeatmap->get_difficultyRank(),
                difficultyBeatmap->get_noteJumpMovementSpeed(),
                difficultyBeatmap->get_noteJumpStartBeatOffset(),
                reinterpret_cast<IPreviewBeatmapLevel*>(difficultyBeatmap->get_level())->get_beatsPerMinute(),
                reinterpret_cast<CustomDifficultyBeatmap*>(difficultyBeatmap)->get_beatmapSaveData(),
                beatmapDataBasicInfo
            );

            customDifficulties[i] = customBeatmap;
        }

        return customDifficulties;
    }

    static void AddCustomCharacteristic(IBeatmapLevel* level) {

        auto difficultyBeatmapSets = ::System::Collections::Generic::List_1<IDifficultyBeatmapSet*>::New_ctor();
        auto originalBeatmapSets = listToArrayW<IDifficultyBeatmapSet*>(level->get_beatmapLevelData()->get_difficultyBeatmapSets());

        for (int i = 0; i < originalBeatmapSets.Length(); i++) {
            difficultyBeatmapSets->Add(originalBeatmapSets[i]);
        }

        for (int i = 0; i < originalBeatmapSets.Length(); i++) {
            IDifficultyBeatmapSet* set = originalBeatmapSets[i];

            if (set->get_beatmapCharacteristic()->get_serializedName() != "Standard") continue;

            auto beatmapSet = CustomDifficultyBeatmapSet::New_ctor(customCharacteristics);
            auto customDifficulties = CreateCustomDifficulties(set->get_difficultyBeatmaps(), beatmapSet);
            beatmapSet->SetCustomDifficultyBeatmaps(customDifficulties);
            if (listToArrayW<IDifficultyBeatmap*>(beatmapSet->get_difficultyBeatmaps()).Length() > 0) {
                difficultyBeatmapSets->Add(reinterpret_cast<IDifficultyBeatmapSet*>(beatmapSet));
            }
        }

        BeatmapLevelData* levelData = reinterpret_cast<BeatmapLevelData*>(level->get_beatmapLevelData());
        levelData->difficultyBeatmapSets = difficultyBeatmapSets->i_IReadOnlyList_1_T();
    }   

    MAKE_HOOK_MATCH(
            SetContent, 
            &StandardLevelDetailView::SetContent, 
            void, 
            StandardLevelDetailView* self, 
            IBeatmapLevel* level, 
            BeatmapDifficulty defaultDifficulty, 
            BeatmapCharacteristicSO* defaultBeatmapCharacteristic, 
            PlayerData* playerData) {
                getLogger().info("0");

        bool alreadyInstalled = il2cpp_utils::try_cast<::System::Collections::Generic::List_1<IDifficultyBeatmapSet*>>(level->get_beatmapLevelData()->get_difficultyBeatmapSets()) != nullopt;

        if (!alreadyInstalled) {
            auto beatMapSets = listToArrayW<IDifficultyBeatmapSet*>(level->get_beatmapLevelData()->get_difficultyBeatmapSets());
            for (int i = 0; i < beatMapSets.Length(); i++)
            {
                IDifficultyBeatmapSet* set = beatMapSets[i];
                if (set->get_beatmapCharacteristic()->get_serializedName() == customCharacteristics->get_serializedName()) {
                    alreadyInstalled = true;
                    break;
                }
            }
        }
        
        IPreviewBeatmapLevel* levelData = reinterpret_cast<IPreviewBeatmapLevel*>(level);
        if (levelData->get_levelID().starts_with("custom_level") && !alreadyInstalled) {
            AddCustomCharacteristic(level);
        }

        SetContent(self, level, defaultDifficulty, defaultBeatmapCharacteristic, playerData);
    }

    MAKE_HOOK_MATCH(
            GetBeatmapCharacteristicBySerializedName, 
            &BeatmapCharacteristicCollectionSO::GetBeatmapCharacteristicBySerializedName, 
            BeatmapCharacteristicSO*, 
            BeatmapCharacteristicCollectionSO* self, 
            ::StringW serializedName) {

                if (serializedName == customCharacteristics->get_serializedName()) {
                    return customCharacteristics;
                } else {
                    return GetBeatmapCharacteristicBySerializedName(self, serializedName);
                }

    }

    MAKE_HOOK_MATCH(
        TransitionSetupDataInit, 
        &StandardLevelScenesTransitionSetupDataSO::Init, 
        void, 
        StandardLevelScenesTransitionSetupDataSO* self, 
        ::StringW gameMode, 
        ::GlobalNamespace::IDifficultyBeatmap* difficultyBeatmap, 
        ::GlobalNamespace::IPreviewBeatmapLevel* previewBeatmapLevel, 
        ::GlobalNamespace::OverrideEnvironmentSettings* overrideEnvironmentSettings, 
        ::GlobalNamespace::ColorScheme* overrideColorScheme, 
        ::GlobalNamespace::GameplayModifiers* gameplayModifiers, 
        ::GlobalNamespace::PlayerSpecificSettings* playerSpecificSettings, 
        ::GlobalNamespace::PracticeSettings* practiceSettings, 
        ::StringW backButtonText, 
        bool useTestNoteCutSoundEffects, 
        bool startPaused) {

            if (difficultyBeatmap->get_parentDifficultyBeatmapSet()->get_beatmapCharacteristic()->get_serializedName() == customCharacteristics->get_serializedName()) {
                customCharacterisitic = CustomCharacterisitic::betterScoring;
            } else {
                customCharacterisitic = CustomCharacterisitic::standard;
            }

            TransitionSetupDataInit(self, gameMode, difficultyBeatmap, previewBeatmapLevel, overrideEnvironmentSettings, overrideColorScheme, gameplayModifiers, playerSpecificSettings, practiceSettings, backButtonText, useTestNoteCutSoundEffects, startPaused);
    }

    void InstallCharacteristics() {

        customCharacteristics = ScriptableObject::CreateInstance<BeatmapCharacteristicSO *>();
        
        customCharacteristics->icon = Sprites::get_RhythmGameIcon();
        customCharacteristics->descriptionLocalizationKey = "It's a rhythm game!";
        customCharacteristics->serializedName = "RhythmGameStandard";
        customCharacteristics->characteristicNameLocalizationKey = "RhythmGameStandard";
        customCharacteristics->compoundIdPartName = "RhythmGameStandard";
        customCharacteristics->requires360Movement = false;
        customCharacteristics->containsRotationEvents = false;
        customCharacteristics->sortingOrder = 99;

        LoggerContextObject logger = getLogger().WithContext("load");

        INSTALL_HOOK(logger, SetContent);
        INSTALL_HOOK(logger, TransitionSetupDataInit);
        INSTALL_HOOK(logger, GetBeatmapCharacteristicBySerializedName);
    }
}