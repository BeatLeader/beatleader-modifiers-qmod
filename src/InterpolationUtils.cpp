#include "include/InterpolationUtils.hpp"

#include "UnityEngine/Mathf.hpp"

namespace InterpolationUtils {

    FrameData::FrameData(float time, Vector3 saberPosition, Vector3 saberDirection, Vector3 notePosition) {
        Time = time;
        SaberPosition = saberPosition;
        SaberDirection = saberDirection;
        NotePosition = notePosition;
    }

    FrameData FrameData::Lerp(FrameData a, FrameData b, float t) {
        return FrameData(
            Mathf::LerpUnclamped(a.Time, b.Time, t),
            Vector3::LerpUnclamped(a.SaberPosition, b.SaberPosition, t),
            Vector3::LerpUnclamped(a.SaberDirection, b.SaberDirection, t).get_normalized(),
            Vector3::LerpUnclamped(a.NotePosition, b.NotePosition, t)
        );
    }
}