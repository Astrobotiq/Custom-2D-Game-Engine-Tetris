//
// Created by e-r-e on 5/20/2026.
//

#ifndef SIMPLEENGINE2D_APPSETTINGS_HPP
#define SIMPLEENGINE2D_APPSETTINGS_HPP

#endif //SIMPLEENGINE2D_APPSETTINGS_HPP
#pragma once
#include <string>

struct AppSettings {
    bool        fullscreen    { false };
    bool        sfxEnabled    { true  };
    bool        musicEnabled  { true  };
    int         activePalette { 0     };
    std::string playerName    { "AAA" };
};