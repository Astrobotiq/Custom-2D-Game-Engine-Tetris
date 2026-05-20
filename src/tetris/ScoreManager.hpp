//
// Created by e-r-e on 5/20/2026.
//

#ifndef SIMPLEENGINE2D_SCOREMANAGER_HPP
#define SIMPLEENGINE2D_SCOREMANAGER_HPP

#endif //SIMPLEENGINE2D_SCOREMANAGER_HPP
#pragma once
#include <string>
#include <vector>

class ScoreManager {
public:
    struct Entry {
        std::string name;
        int         score;
        int         level;
        int         lines;
    };

    // Dosyaya metin formatında (CSV) kaydet
    static void save(const std::vector<Entry>& entries, const std::string& path);

    // Dosyadan yükle, yoksa veya bozuksa boş/sağlam olanları döndür
    static std::vector<Entry> load(const std::string& path);

    // Yeni skoru ekle, sırala ve sadece ilk 10'u tut
    static void addEntry(std::vector<Entry>& entries, Entry e);
};