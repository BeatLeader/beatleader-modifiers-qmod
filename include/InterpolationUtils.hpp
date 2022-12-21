#pragma once
#include "UnityEngine/Vector3.hpp"

using namespace UnityEngine;

namespace InterpolationUtils {

    struct FrameData {
        float Time;
        Vector3 SaberPosition;
        Vector3 SaberDirection;
        Vector3 NotePosition;

        FrameData(float time, Vector3 saberPosition, Vector3 saberDirection, Vector3 notePosition);

        static FrameData Lerp(FrameData a, FrameData b, float t);
    };

    inline float GetDistance(Vector3 lineFrom, Vector3 lineDirection, Vector3 point) {
        Vector3 v = point - lineFrom;
        float angle = Vector3::Angle(v, lineDirection) * 0.01745;
        return sin(angle) * v.get_magnitude();
    }

    inline void CalculateClosestApproach(FrameData a, FrameData b, float* time, float* distance) {
        *time = 0.0f;
        *distance = 10000;
        int Resolution = 200;

        for (int i = 0; i <= Resolution; i++) {
            float t = (float)i / (Resolution);
            t = -1 + 3 * t;
            
            FrameData f = FrameData::Lerp(a, b, t);
            float d = GetDistance(f.SaberPosition, f.SaberDirection, f.NotePosition);
            if (d >= *distance) continue;
            *time = f.Time;
            *distance = d;
        }
    }
}