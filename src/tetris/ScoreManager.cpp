//
// Created by e-r-e on 5/20/2026.
//
#include "ScoreManager.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>

void ScoreManager::save(const std::vector<Entry>& entries, const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) return;

    for (const auto& e : entries) {
        file << e.name << "," << e.score << "," << e.level << "," << e.lines << "\n";
    }
}

std::vector<ScoreManager::Entry> ScoreManager::load(const std::string& path) {
    std::vector<Entry> entries;
    std::ifstream file(path);
    if (!file.is_open()) return entries;

    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string name, scoreStr, levelStr, linesStr;

        if (std::getline(ss, name, ',') &&
            std::getline(ss, scoreStr, ',') &&
            std::getline(ss, levelStr, ',') &&
            std::getline(ss, linesStr)) {

            try {
                entries.push_back({
                    name,
                    std::stoi(scoreStr),
                    std::stoi(levelStr),
                    std::stoi(linesStr)
                });
            } catch (...) {
                // Parse hatası olan satırları atla
                continue;
            }
            }
    }
    return entries;
}

void ScoreManager::addEntry(std::vector<Entry>& entries, Entry e) {
    entries.push_back(e);

    std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) {
        return a.score > b.score;
    });

    if (entries.size() > 10) {
        entries.resize(10);
    }
}