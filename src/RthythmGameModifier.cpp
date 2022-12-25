#include "include/RthythmGameModifier.hpp"
#include "include/CharacteristicsManager.hpp"
#include "include/InterpolationUtils.hpp"
#include "include/ModConfig.hpp"

#include "UnityEngine/Vector3.hpp"
#include "UnityEngine/Transform.hpp"
#include "UnityEngine/Mathf.hpp"

#include "GlobalNamespace/NoteController.hpp"
#include "GlobalNamespace/AudioTimeSyncController.hpp"
#include "GlobalNamespace/ScoreController.hpp"
#include "GlobalNamespace/NoteCutInfo.hpp"
#include "GlobalNamespace/BeatmapObjectManager.hpp"
#include "GlobalNamespace/NoteData.hpp"
#include "GlobalNamespace/SaberMovementData.hpp"
#include "GlobalNamespace/CutScoreBuffer.hpp"
#include "GlobalNamespace/SaberSwingRatingCounter.hpp"

#include "GlobalNamespace/GameNoteController.hpp"
#include "GlobalNamespace/CuttableBySaber.hpp"
#include "GlobalNamespace/BoxCuttableBySaber.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "beatsaber-hook/shared/utils/logging.hpp"

#include "System/Threading/Tasks/Task_1.hpp"

#include "main.hpp"

#include <map>

using namespace std;
using namespace UnityEngine;
using namespace GlobalNamespace;

namespace BeatLeaderModifiers {
    struct NoteMovementData {
        float SongTime;
        Vector3 NotePosition;

        NoteMovementData(float songTime, Vector3 notePosition);
        NoteMovementData() = default;
    };

    NoteMovementData::NoteMovementData(float songTime, Vector3 notePosition) {
        SongTime = songTime;
        NotePosition = notePosition;
    }

    map<NoteController *, NoteMovementData> noteMovementCache;
    map<NoteController *, NoteMovementData> noteMovementCache2;
    AudioTimeSyncController* audioTimeSyncController;

    float badTiming = 0.04;
    float goodTiming = 0.035;

    MAKE_HOOK_MATCH(
        NoteCut, 
        &NoteController::SendNoteWasCutEvent, 
        void, 
        NoteController* self, 
        ByRef<NoteCutInfo> noteCutInfo) {

        if (UploadDisabledByReplay() || customCharacterisitic != CustomCharacterisitic::betterScoring || !noteMovementCache.contains(self)) { 
            NoteCut(self, noteCutInfo);
            return;
        }

        NoteCutInfo derefCutInfo = noteCutInfo.heldRef;

        auto previousNoteMovementData = noteMovementCache[self];
        auto currentNotePosition = derefCutInfo.notePosition;

        auto saberMovementData = reinterpret_cast<SaberMovementData*>(derefCutInfo.saberMovementData);
        auto previousBladeData = saberMovementData->get_prevAddedData();
        auto currentBladeData = saberMovementData->get_lastAddedData();

        auto previousFrameData = InterpolationUtils::FrameData(
            previousNoteMovementData.SongTime,
            previousBladeData.bottomPos,
            previousBladeData.topPos - previousBladeData.bottomPos,
            previousNoteMovementData.NotePosition
        );

        auto currentFrameData = InterpolationUtils::FrameData(
            audioTimeSyncController->get_songTime(),
            currentBladeData.bottomPos,
            currentBladeData.topPos - currentBladeData.bottomPos,
            currentNotePosition
        );

        float newTime;
        float newDistance;

        InterpolationUtils::CalculateClosestApproach(previousFrameData, currentFrameData, &newTime, &newDistance);
        float newTimeDeviation = derefCutInfo.noteData->time - newTime;

        float timingRating = Mathf::Clamp01((Mathf::Abs(newTimeDeviation) - goodTiming) / badTiming);

        *noteCutInfo = NoteCutInfo(
            derefCutInfo.noteData,
            derefCutInfo.speedOK,
            derefCutInfo.directionOK,
            derefCutInfo.saberTypeOK,
            derefCutInfo.wasCutTooSoon,
            derefCutInfo.saberSpeed,
            derefCutInfo.saberDir,
            derefCutInfo.saberType,
            newTimeDeviation,
            derefCutInfo.cutDirDeviation,
            derefCutInfo.cutPoint,
            derefCutInfo.cutNormal,
            derefCutInfo.cutAngle,
            timingRating / 3.0,
            derefCutInfo.worldRotation,
            derefCutInfo.inverseWorldRotation,
            derefCutInfo.noteRotation,
            derefCutInfo.notePosition,
            derefCutInfo.saberMovementData
        );

        NoteCut(self, noteCutInfo);
    }

    MAKE_HOOK_MATCH(CutScoreBufferRefreshScores, &CutScoreBuffer::RefreshScores, void, CutScoreBuffer* self) {
        if (!UploadDisabledByReplay() && customCharacterisitic == CustomCharacterisitic::betterScoring) {
            float timingRating = 1 - Mathf::Clamp01((Mathf::Abs(self->noteCutInfo.timeDeviation) - goodTiming) / badTiming);
            self->saberSwingRatingCounter->afterCutRating = timingRating;
            self->saberSwingRatingCounter->beforeCutRating = timingRating;
        }

        CutScoreBufferRefreshScores(self);
    }

    void scaleCollider(CuttableBySaber *cuttable, float value) {
        auto transform = cuttable->get_transform();

        auto localScale = transform->get_localScale();
        localScale.x *= value;
        localScale.y *= value;
        localScale.z *= value;

        transform->set_localScale(localScale);
    }

    MAKE_HOOK_MATCH(NoteControllerInit, &NoteController::Init, void, NoteController* self, NoteData* noteData, float worldRotation, Vector3 moveStartPos, Vector3 moveEndPos, Vector3 jumpEndPos, float moveDuration, float jumpDuration, float jumpGravity, float endRotation, float uniformScale, bool rotatesTowardsPlayer, bool useRandomRotation) {
        NoteControllerInit(self, noteData, worldRotation, moveStartPos, moveEndPos, jumpEndPos, moveDuration, jumpDuration, jumpGravity, endRotation, uniformScale, rotatesTowardsPlayer, useRandomRotation);
        if (UploadDisabledByReplay() || customCharacterisitic != CustomCharacterisitic::betterScoring) {
            return;
        }
        float colliderScale = 0.58;
        auto gameNote = il2cpp_utils::try_cast<GameNoteController>(self);
        if (gameNote != std::nullopt) {
            auto bigCuttable = gameNote.value()->bigCuttableBySaberList;
            for (size_t i = 0; i < bigCuttable.Length(); i++)
            {
                scaleCollider(bigCuttable[i], colliderScale);
            }

            auto smallCuttable = gameNote.value()->smallCuttableBySaberList;
            for (size_t i = 0; i < smallCuttable.Length(); i++)
            {
                scaleCollider(smallCuttable[i], colliderScale);
            }
        }
    }

    MAKE_HOOK_MATCH(SongStart, &AudioTimeSyncController::Start, void, AudioTimeSyncController* self) {
        SongStart(self);
        audioTimeSyncController = self;
    }

    MAKE_HOOK_MATCH(
        NoteControllerUpdate, 
        &NoteController::ManualUpdate,
        void,
        NoteController* self) {
            NoteControllerUpdate(self);

            if (!UploadDisabledByReplay() && audioTimeSyncController && self->get_transform()) { 
                if (noteMovementCache2.contains(self)) {
                    noteMovementCache[self] = noteMovementCache2[self];
                }
                noteMovementCache2[self] = NoteMovementData(audioTimeSyncController->get_songTime(), self->get_transform()->get_position());
            }
        }

    void InstallRthythmGame() {
        LoggerContextObject logger = getLogger().WithContext("load");

        INSTALL_HOOK(logger, NoteCut);
        INSTALL_HOOK(logger, SongStart);
        INSTALL_HOOK(logger, NoteControllerUpdate);
        INSTALL_HOOK(logger, CutScoreBufferRefreshScores);
        INSTALL_HOOK(logger, NoteControllerInit);
    }
}