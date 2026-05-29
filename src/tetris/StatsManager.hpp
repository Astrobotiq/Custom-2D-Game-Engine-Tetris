#pragma once
#include <string>
#include <vector>
#include <map>
#include "AppSettings.hpp"

namespace tetris {
    class StatsManager {
    public:
        struct ModeStats {
            int playCount{0};
            std::vector<int> history; // chronological list of scores or remaining blocks
        };

        static std::map<GameMode, ModeStats> load(const std::string& path);
        static void save(const std::map<GameMode, ModeStats>& stats, const std::string& path);
        static void addRecord(std::map<GameMode, ModeStats>& stats, GameMode mode, int value);
        static std::string formatValue(GameMode mode, int value);
    };
} // namespace tetris
