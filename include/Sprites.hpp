#pragma once

#include <string>

#include "UnityEngine/Sprite.hpp"

using namespace std;
/// <summary>
/// Helper code taken from BeatSaverDownloader
/// Copyright (c) 2018 andruzzzhka (MIT Licensed)
/// </summary>
class Sprites {
    static const string RhythmGameB64;
public:
    static UnityEngine::Sprite* get_RhythmGameIcon();
};