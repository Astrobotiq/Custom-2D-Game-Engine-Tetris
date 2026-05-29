//
// Created by e-r-e on 5/20/2026.
//

#ifndef SIMPLEENGINE2D_APPSETTINGS_HPP
#define SIMPLEENGINE2D_APPSETTINGS_HPP

#endif //SIMPLEENGINE2D_APPSETTINGS_HPP
#pragma once
#include <string>

enum class GameMode {
    Classic,
    GarbageClear,
    GarbageTimeAttack,
    ScoreTimeAttack,
    Ascension,
    TacticalStep,
    TacticalGarbageClear,
    Custom
};

struct CustomModeSettings {
    bool gravityEnabled{true};
    bool tacticalStepEnabled{false};
    bool garbageBoardEnabled{false};
    float garbageRatio{0.40f};
    bool timeLimitEnabled{false};
    float timeLimit{120.f};
    bool targetScoreEnabled{false};
    int targetScore{2000};
    bool slotMachineEnabled{true};
    bool specialBlocksEnabled{true};
    bool ascensionEnabled{false};
};

inline std::string getScorePath(GameMode mode) {
    switch (mode) {
        case GameMode::Classic: return "assets/scores_classic.csv";
        case GameMode::GarbageClear: return "assets/scores_garbageclear.csv";
        case GameMode::GarbageTimeAttack: return "assets/scores_garbagetime.csv";
        case GameMode::ScoreTimeAttack: return "assets/scores_scoretime.csv";
        case GameMode::Ascension: return "assets/scores_ascension.csv";
        case GameMode::TacticalStep: return "assets/scores_tacticalstep.csv";
        case GameMode::TacticalGarbageClear: return "assets/scores_tacticalgarbage.csv";
        case GameMode::Custom: return "assets/scores_custom.csv";
        default: return "assets/scores.csv";
    }
}

inline std::string getModeName(GameMode mode) {
    switch (mode) {
        case GameMode::Classic: return "CLASSIC";
        case GameMode::GarbageClear: return "GARBAGE CLEAR";
        case GameMode::GarbageTimeAttack: return "GARBAGE TIME";
        case GameMode::ScoreTimeAttack: return "SCORE TIME";
        case GameMode::Ascension: return "ASCENSION";
        case GameMode::TacticalStep: return "TACTICAL STEP";
        case GameMode::TacticalGarbageClear: return "TACTICAL GARBAGE";
        case GameMode::Custom: return "CUSTOM";
        default: return "UNKNOWN";
    }
}

struct AppSettings {
    bool        fullscreen    { false };
    bool        sfxEnabled    { true  };
    bool        musicEnabled  { true  };
    int         activePalette { 0     };
    std::string playerName    { "AAA" };
    GameMode    activeMode    { GameMode::Classic };
    float       garbageRatio  { 0.40f };
    float       scoreTimeLimit { 120.f };
    int         scoreTimeTarget{ 2000 };
    CustomModeSettings customSettings;
};