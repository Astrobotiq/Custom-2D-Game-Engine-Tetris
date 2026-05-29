#include "StatsManager.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdio>

namespace tetris {
    std::map<GameMode, StatsManager::ModeStats> StatsManager::load(const std::string& path) {
        std::map<GameMode, ModeStats> stats;
        
        // Pre-initialize map keys for all 8 modes
        for (int i = 0; i <= 7; ++i) {
            stats[static_cast<GameMode>(i)] = ModeStats{0, {}};
        }

        std::ifstream file(path);
        if (!file.is_open()) return stats;

        std::string line;
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string modeStr, countStr;
            if (std::getline(ss, modeStr, ',') && std::getline(ss, countStr, ',')) {
                try {
                    GameMode mode = static_cast<GameMode>(std::stoi(modeStr));
                    int count = std::stoi(countStr);
                    
                    std::vector<int> history;
                    std::string valStr;
                    while (std::getline(ss, valStr, ',')) {
                        if (!valStr.empty()) {
                            history.push_back(std::stoi(valStr));
                        }
                    }
                    
                    stats[mode] = ModeStats{count, history};
                } catch (...) {
                    continue;
                }
            }
        }
        return stats;
    }

    void StatsManager::save(const std::map<GameMode, ModeStats>& stats, const std::string& path) {
        std::ofstream file(path);
        if (!file.is_open()) return;

        for (const auto& pair : stats) {
            file << static_cast<int>(pair.first) << "," << pair.second.playCount;
            for (int val : pair.second.history) {
                file << "," << val;
            }
            file << "\n";
        }
    }

    void StatsManager::addRecord(std::map<GameMode, ModeStats>& stats, GameMode mode, int value) {
        auto& modeStats = stats[mode];
        modeStats.playCount++;
        modeStats.history.push_back(value);
    }

    std::string StatsManager::formatValue(GameMode mode, int value) {
        if (mode == GameMode::Classic || mode == GameMode::Ascension || mode == GameMode::TacticalStep || mode == GameMode::Custom) {
            return std::to_string(value) + " PTS";
        }
        else if (mode == GameMode::ScoreTimeAttack) {
            if (value >= 10000) {
                float timeLeft = (value - 10000) / 10.f;
                char buf[32];
                std::snprintf(buf, sizeof(buf), "Cleared (%.1fs left)", timeLeft);
                return buf;
            } else {
                return "Failed (" + std::to_string(value) + " PTS)";
            }
        }
        else { // GarbageClear, GarbageTimeAttack, TacticalGarbageClear
            if (value < 5000) {
                int mins = value / 60;
                int secs = value % 60;
                char buf[32];
                std::snprintf(buf, sizeof(buf), "Cleared (%02d:%02d)", mins, secs);
                return buf;
            } else {
                int remaining = value - 5000;
                return "Failed (" + std::to_string(remaining) + " left)";
            }
        }
    }
} // namespace tetris
