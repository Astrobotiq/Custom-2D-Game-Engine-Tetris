#pragma once

#include "Board.hpp"
#include "Tetromino.hpp"
#include "SlotMachine.hpp"
#include "AppSettings.hpp"
#include <random>
#include <array>
#include <optional>
#include <functional>
#include <climits>

namespace tetris {
    inline constexpr int SELECTION_SIZE = 3;
    inline constexpr std::array<int, 5> LINE_SCORES = {0, 100, 300, 500, 800};

    // ─────────────────────────────────────────────────────────────────────────────
    // BoardAnalysis
    // ─────────────────────────────────────────────────────────────────────────────
    struct BoardAnalysis {
        std::array<int, BOARD_COLS> colHeights{};
        float averageHeight{0.f};
        int bumpiness{0};
        int holes{0};
        int emptiest_col_start{0};
        int emptiest_col_end{BOARD_COLS - 1};
    };

    // ─────────────────────────────────────────────────────────────────────────────
    struct GameState {
        Board board;
        Piece fallingPiece;
        bool gameOver{false};
        bool paused{false};
        bool debugDisableGravity{false};
        bool fallingFrozen{false}; // Slot machine açıkken dondur

        // Zamanı Durdurma Sayacı
        float timeStopTimer{0.f};

        // Level Geçiş Dondurması (15 sn boyunca parça düşmez, ama SelectionBar çalışır)
        float levelTransitionTimer{0.f};
        bool levelTransitionActive{false};
        int lastTrackedLevel{1}; // Önceki level — değişimi algılamak için

        // Düşen parçanın jeton taşıyıp taşımadığı
        bool fallingHasToken{false};

        std::array<Piece, SELECTION_SIZE> selectionPieces;

        int score{0};
        int linesCleared{0};
        int level{1};
        float fallInterval{1.0f};
        float fallAccum{0.f};
        int scoreTokenMilestone{0}; // Son jeton verilen puan eşiği (1000'in katları)

        // Bir sonraki parçanın (nextPieces[0]) düşeceği sütun — spawn anında bu değer kullanılır
        int nextPieceCol{BOARD_COLS / 2 - 2};

        std::array<Piece, 3> nextPieces;

        // Slot machine sistemi
        SlotMachine slotMachine;

        std::function<void(int, std::vector<int>)> onLinesCleared;
        std::function<void()> onPieceLocked;
        std::function<void()> onGameOver;
        std::function<void(int col, int row, sf::Color color)> onCellCleared;
        // Slot machine sonucu döndüğünde: güç kazanıldıysa bildir
        std::function<void(SlotResult)> onSlotResult;
        std::function<void(FreeSlotResult)> onFreeSlotResult;
        std::function<void(int col, int row)> onBombExploded;

        // Mod Değişkenleri
        GameMode gameMode{GameMode::Classic};
        bool gameWon{false};
        float countdownTimer{120.f};
        int targetScore{2000};
        int ascensionTargetScore{1500};
        std::function<void()> onGameWon;
        CustomModeSettings customSettings;

        explicit GameState(std::uint32_t seed = 42)
            : rng(seed) {
            board.onCellCleared = [this](int col, int row, sf::Color color) {
                if (onCellCleared) onCellCleared(col, row, color);
            };
            board.onBombExploded = [this](int col, int row) {
                if (onBombExploded) onBombExploded(col, row);
            };
            for (auto &p: nextPieces) p = spawnPieceRandom();
            refreshSelectionPieces();
            // İlk spawnFallingPiece nextPieceCol'u kullanacak, önceden hesapla
            nextPieceCol = chooseFallingCol(nextPieces[0]);
            spawnFallingPiece();
        }

        // ── Güncelleme ───────────────────────────────────────────────────────────
        void update(float dt) {
            if (gameOver || paused || gameWon) return;

            // Zamanı durdurma gücü aktifse sayacı azalt
            if (timeStopTimer > 0.f) {
                timeStopTimer -= dt;
                // Sayacın eksiye düşmesini engelle
                if (timeStopTimer < 0.f) timeStopTimer = 0.f;
            }

            // Geri sayım sayacını yönet (TimeAttack veya Ascension modlarında)
            bool hasTimeLimit = (gameMode == GameMode::GarbageTimeAttack || 
                                 gameMode == GameMode::ScoreTimeAttack || 
                                 gameMode == GameMode::Ascension ||
                                 (gameMode == GameMode::Custom && customSettings.timeLimitEnabled));
            if (hasTimeLimit) {
                if (timeStopTimer <= 0.f && !levelTransitionActive) {
                    countdownTimer -= dt;
                    if (countdownTimer <= 0.f) {
                        countdownTimer = 0.f;
                        gameOver = true;
                        if (onGameOver) onGameOver();
                        return;
                    }
                }
            } else if (gameMode == GameMode::TacticalGarbageClear || 
                       gameMode == GameMode::GarbageClear ||
                       (gameMode == GameMode::Custom && !customSettings.timeLimitEnabled)) {
                if (timeStopTimer <= 0.f && !levelTransitionActive) {
                    countdownTimer += dt;
                }
            }

            // Level geçiş sayacını azalt
            if (levelTransitionActive) {
                levelTransitionTimer -= dt;
                if (levelTransitionTimer <= 0.f) {
                    levelTransitionTimer = 0.f;
                    levelTransitionActive = false;
                }
            }

            // Eğer zaman durmamışsa, dondurulmamışsa, level geçişi yoksa, sıra tabanlı değilse ve yerçekimi açıksa düşmeye devam et
            bool isTactical = (gameMode == GameMode::TacticalStep || 
                               gameMode == GameMode::TacticalGarbageClear ||
                               (gameMode == GameMode::Custom && !customSettings.gravityEnabled));
            if (!debugDisableGravity && !fallingFrozen && timeStopTimer <= 0.f && !levelTransitionActive && !isTactical) {
                fallAccum += dt;
                if (fallAccum >= fallInterval) {
                    fallAccum -= fallInterval;
                    stepFallingPiece();
                }
            }
        }

        // ── Zamanı Durdurma Gücü ─────────────────────────────────────────────────
        void applyTimeStop(float seconds) {
            timeStopTimer += seconds;
        }

        // ── Seçim parçasını yerleştir ────────────────────────────────────────────
        bool placeSelectionPiece(int selectionIndex, int targetCol, int targetRow) {
            if (selectionIndex < 0 || selectionIndex >= SELECTION_SIZE) return false;

            Piece piece = selectionPieces[selectionIndex];

            auto [valid, placed] = board.getDirectPlacement(piece, targetCol, targetRow, &fallingPiece);
            if (!valid) return false;

            board.place(placed);
            processLineClears();
            refreshSelectionPieces();

            bool isTactical = (gameMode == GameMode::TacticalStep || 
                               gameMode == GameMode::TacticalGarbageClear ||
                               (gameMode == GameMode::Custom && customSettings.tacticalStepEnabled));
            if (isTactical) {
                int stepAmount = 1;
                if (level >= 5) stepAmount = 3;
                else if (level >= 3) stepAmount = 2;
                for (int i = 0; i < stepAmount && !gameOver && !gameWon; ++i) {
                    stepFallingPiece();
                }
            }

            return true;
        }

        void rotateSelectionPiece(int index) {
            if (index >= 0 && index < SELECTION_SIZE)
                selectionPieces[index].rotateRight();
        }

        // ── Slot Machine: Reroll ─────────────────────────────────────────────────
        // Oyuncu "ÇEVİR" butonuna basınca çağrılır.
        // Döndürür: false = yeterli jeton yok
        bool reroll() {
            if (!slotMachine.canReroll()) return false;
            slotMachine.tokens -= SlotMachine::REROLL_COST;

            refreshSelectionPiecesRandom();

            std::array<TetrominoType, 3> types = {
                selectionPieces[0].type,
                selectionPieces[1].type,
                selectionPieces[2].type
            };

            SlotResult result = slotMachine.evaluateCombo(types);

            bool isTactical = (gameMode == GameMode::TacticalStep || gameMode == GameMode::TacticalGarbageClear);
            if (!isTactical) {
                fallingPiece.row = 0;
            }
            fallAccum = 0.f;

            if (onSlotResult) onSlotResult(result);
            return true;
        }

        // ── Güç kullan ───────────────────────────────────────────────────────────
        // TetrisScene güç tipine göre ne yapacağını bilir,
        // burada sadece "kullanıldı" işaretlenir.
        // Jetonsuz slot yenileme — T gücü için
        void freeReroll(int slotCount) {
            if (slotCount >= SELECTION_SIZE) {
                refreshSelectionPiecesRandom(); // Tüm slotları yenile
            } else {
                for (int i = 0; i < slotCount && i < SELECTION_SIZE; ++i)
                    selectionPieces[i] = spawnPieceRandom();
            }
            std::array<TetrominoType, 3> types = {
                selectionPieces[0].type,
                selectionPieces[1].type,
                selectionPieces[2].type
            };
            FreeSlotResult result = {types};

            if (onFreeSlotResult) onFreeSlotResult(result);
        }

        PowerType activatePower(int index) {
            if (index < 0 || index >= static_cast<int>(slotMachine.powers.size()))
                return PowerType::None;
            PowerType t = slotMachine.powers[index].type;
            slotMachine.usePower(index);
            slotMachine.cleanUsedPowers();
            return t;
        }

        // Düşen parçayı dondur/çöz (slot machine açılınca)
        void freezeFalling(bool freeze) {
            fallingFrozen = freeze;
            if (!freeze) fallAccum = 0.f; // Çözülünce sayacı sıfırla
        }

        void setupGarbageBoard(float fillRatio = 0.40f) {
            board = Board();
            std::uniform_int_distribution<int> colorDist(1, 7);
            std::uniform_real_distribution<float> fillDist(0.f, 1.f);
            
            std::array<sf::Color, 7> pieceColors = {
                sf::Color(0, 240, 240), // I
                sf::Color(240, 240, 0), // O
                sf::Color(160, 0, 240), // T
                sf::Color(0, 240, 0),   // S
                sf::Color(240, 0, 0),   // Z
                sf::Color(0, 0, 240),   // J
                sf::Color(240, 160, 0)  // L
            };

            for (int r = BOARD_ROWS - 8; r < BOARD_ROWS; ++r) {
                for (int c = 0; c < BOARD_COLS; ++c) {
                    if (fillDist(rng) < fillRatio) {
                        int colorIdx = colorDist(rng) - 1;
                        board.setCell(c, r, pieceColors[colorIdx]);
                        board.setOriginalGarbage(c, r, true);

                        // Spawn special blocks
                        bool spawnSpecial = (gameMode != GameMode::Custom || customSettings.specialBlocksEnabled);
                        if (spawnSpecial) {
                            std::uniform_real_distribution<float> specialDist(0.f, 1.f);
                            float val = specialDist(rng);
                            if (val < 0.10f) {
                                std::uniform_int_distribution<int> countDist(2, 3);
                                board.setSpecialTile(c, r, {SpecialTileType::Counter, countDist(rng)});
                            } else if (val < 0.15f) {
                                board.setSpecialTile(c, r, {SpecialTileType::Bomb, 0});
                            }
                        }
                    }
                }
                
                bool allEmpty = true;
                bool allFilled = true;
                for (int c = 0; c < BOARD_COLS; ++c) {
                    if (!board.isEmpty(c, r)) allEmpty = false;
                    else allFilled = false;
                }
                if (allFilled) {
                    int holeCol = std::uniform_int_distribution<int>(0, BOARD_COLS - 1)(rng);
                    board.setCell(holeCol, r, EMPTY_COLOR);
                }
                if (allEmpty) {
                    int fillCol = std::uniform_int_distribution<int>(0, BOARD_COLS - 1)(rng);
                    int colorIdx = colorDist(rng) - 1;
                    board.setCell(fillCol, r, pieceColors[colorIdx]);
                    board.setOriginalGarbage(fillCol, r, true);

                    // Spawn special block if this guaranteed fill becomes special
                    bool spawnSpecial = (gameMode != GameMode::Custom || customSettings.specialBlocksEnabled);
                    if (spawnSpecial) {
                        std::uniform_real_distribution<float> specialDist(0.f, 1.f);
                        float val = specialDist(rng);
                        if (val < 0.10f) {
                            std::uniform_int_distribution<int> countDist(2, 3);
                            board.setSpecialTile(fillCol, r, {SpecialTileType::Counter, countDist(rng)});
                        } else if (val < 0.15f) {
                            board.setSpecialTile(fillCol, r, {SpecialTileType::Bomb, 0});
                        }
                    }
                }
            }
        }

        bool isBoardEmpty() const {
            for (int r = 0; r < BOARD_ROWS; ++r) {
                for (int c = 0; c < BOARD_COLS; ++c) {
                    if (!board.isEmpty(c, r)) return false;
                }
            }
            return true;
        }

        // ── Board analizi ile güç uygulama ───────────────────────────────────────
        void applyColumnLower(int colCount, int amount) {
            BoardAnalysis a = analyzeBoard();
            std::vector<int> sortedCols(BOARD_COLS);
            for (int i = 0; i < BOARD_COLS; ++i) sortedCols[i] = i;
            std::sort(sortedCols.begin(), sortedCols.end(), [&](int a_, int b_) {
                return a.colHeights[a_] > a.colHeights[b_];
            });
            for (int n = 0; n < colCount && n < BOARD_COLS; ++n) {
                int col = sortedCols[n];
                for (int step = 0; step < amount; ++step) {
                    for (int r = 0; r < BOARD_ROWS; ++r) {
                        if (!board.isEmpty(col, r)) {
                            board.setCell(col, r, EMPTY_COLOR);
                            break;
                        }
                    }
                }
            }
        }

        void applyRandomBlockClear(int count) {
            std::vector<std::pair<int, int> > filled;
            for (int r = 0; r < BOARD_ROWS; ++r)
                for (int c = 0; c < BOARD_COLS; ++c)
                    if (!board.isEmpty(c, r))
                        filled.emplace_back(c, r);

            std::shuffle(filled.begin(), filled.end(), rng);
            int cleared = std::min(count, static_cast<int>(filled.size()));
            for (int i = 0; i < cleared; ++i) {
                int col = filled[i].first;
                int row = filled[i].second;
                board.setCell(col, row, EMPTY_COLOR);
            }
        }

        void applyClearRow(int row) {
            if (row >= 0 && row < BOARD_ROWS) {
                board.removeRow(row);
            }
        }

        void applyMoveFallingToCol(int newCol) {
            Piece moved = fallingPiece;
            moved.col = std::clamp(newCol, 0, BOARD_COLS - 4);
            if (board.canPlace(moved)) fallingPiece = moved;
        }

        void applyRotateFalling(int times, bool left) {
            for (int i = 0; i < times; ++i) {
                Piece rotated = fallingPiece;
                if (left) rotated.rotateLeft();
                else rotated.rotateRight();
                if (board.canPlace(rotated)) fallingPiece = rotated;
            }
        }

        bool isHardFallAllowed() const {
            return gameMode == GameMode::GarbageTimeAttack || 
                   gameMode == GameMode::ScoreTimeAttack || 
                   gameMode == GameMode::Ascension || 
                   gameMode == GameMode::TacticalStep || 
                   gameMode == GameMode::TacticalGarbageClear ||
                   gameMode == GameMode::Custom;
        }

        void hardDropFallingPiece() {
            Piece next = fallingPiece;
            int dropDistance = 0;
            while (true) {
                next.row++;
                if (!board.canPlace(next)) break;
                fallingPiece = next;
                dropDistance++;
            }

            if (dropDistance > 0) {
                score += dropDistance * 2;
                if (gameMode != GameMode::Ascension) {
                    int newMilestone = (score / 1000);
                    if (newMilestone > scoreTokenMilestone) {
                        int tokensToAdd = newMilestone - scoreTokenMilestone;
                        scoreTokenMilestone = newMilestone;
                        for (int t = 0; t < tokensToAdd; ++t) slotMachine.addToken(1);
                    }
                }
            }

            board.place(fallingPiece);
            if (onPieceLocked) onPieceLocked();
            processLineClears();
            spawnFallingPiece();
            fallAccum = 0.f;
        }

        void recalcLevel() {
            int newLevel = std::max(1, linesCleared / 10 + 1);
            if (newLevel > level) {
                // Level atlandı — 25 saniyelik geçiş dondurması başlat (sadece belirli modlarda)
                bool allowTransition = (gameMode == GameMode::Classic ||
                                        gameMode == GameMode::GarbageClear ||
                                        gameMode == GameMode::GarbageTimeAttack);
                if (allowTransition) {
                    levelTransitionTimer = 25.f;
                    levelTransitionActive = true;
                }

                // YENİ: Düşen parçayı yok etmeden en üste geri gönder
                fallingPiece.row = 0;
                fallAccum = 0.f;
            }
            level = newLevel;
            fallInterval = std::max(0.1f, 1.0f - (level - 1) * 0.08f);
            if (gameMode == GameMode::GarbageClear) {
                fallInterval *= 2.0f;
            }
            if (gameMode == GameMode::Custom && !customSettings.gravityEnabled) {
                fallInterval = 999999.f;
            }
        }

        void spawnFallingPiece() {
            fallingPiece = nextPieces[0];
            fallingPiece.row = 0;
            // Önceki turda hesaplanıp saklanan col'u kullan (okla gösterilen yer)
            fallingPiece.col = nextPieceCol;

            // Jeton şansı: TOKEN_CHANCE % (sadece Ascension modunda değilse)
            if (gameMode != GameMode::Ascension) {
                std::uniform_int_distribution<int> tokenRoll(0, 99);
                fallingHasToken = (tokenRoll(rng) < SlotMachine::TOKEN_CHANCE);
            } else {
                fallingHasToken = false;
            }

            fallingPiece.hasToken = fallingHasToken;

            nextPieces[0] = nextPieces[1];
            nextPieces[1] = nextPieces[2];
            nextPieces[2] = spawnPieceRandom();

            // Yeni nextPieces[0] için col'u şimdiden hesapla ve sakla
            nextPieceCol = chooseFallingCol(nextPieces[0]);

            if (!board.canPlace(fallingPiece)) {
                gameOver = true;
                if (onGameOver) onGameOver();
            }
        }

    private:
        std::mt19937 rng;

        float slotQuality(int slotIndex) const {
            static constexpr std::array<float, SELECTION_SIZE> q = {0.9f, 0.5f, 0.0f};
            return q[slotIndex];
        }

        BoardAnalysis analyzeBoard() const {
            BoardAnalysis a;
            for (int c = 0; c < BOARD_COLS; ++c) {
                a.colHeights[c] = 0;
                for (int r = 0; r < BOARD_ROWS; ++r) {
                    if (!board.isEmpty(c, r)) {
                        a.colHeights[c] = BOARD_ROWS - r;
                        break;
                    }
                }
            }
            int total = 0;
            for (int h: a.colHeights) total += h;
            a.averageHeight = static_cast<float>(total) / BOARD_COLS;

            a.bumpiness = 0;
            for (int c = 0; c < BOARD_COLS - 1; ++c)
                a.bumpiness += std::abs(a.colHeights[c] - a.colHeights[c + 1]);

            a.holes = 0;
            for (int c = 0; c < BOARD_COLS; ++c) {
                bool blockAbove = false;
                for (int r = 0; r < BOARD_ROWS; ++r) {
                    if (!board.isEmpty(c, r)) blockAbove = true;
                    else if (blockAbove) a.holes++;
                }
            }

            int minSum = INT_MAX;
            for (int c = 0; c <= BOARD_COLS - 3; ++c) {
                int sum = a.colHeights[c] + a.colHeights[c + 1] + a.colHeights[c + 2];
                if (sum < minSum) {
                    minSum = sum;
                    a.emptiest_col_start = c;
                    a.emptiest_col_end = c + 2;
                }
            }
            return a;
        }

        float scorePiece(TetrominoType type, int rotation,
                         const BoardAnalysis &a) const {
            Piece tmp;
            tmp.type = type;
            tmp.rotation = rotation;
            tmp.col = 0;
            tmp.row = 0;
            auto cells = tmp.localCells();
            int minC = INT_MAX, maxC = INT_MIN, minR = INT_MAX, maxR = INT_MIN;
            for (auto [r,c]: cells) {
                minC = std::min(minC, c);
                maxC = std::max(maxC, c);
                minR = std::min(minR, r);
                maxR = std::max(maxR, r);
            }
            int pieceWidth = (minC == INT_MAX) ? 1 : (maxC - minC + 1);
            int pieceHeight = (minR == INT_MAX) ? 1 : (maxR - minR + 1);
            float evalScore = 0.f;

            if (a.bumpiness <= 4) {
                if (type == TetrominoType::I && rotation % 2 == 0) evalScore += 40.f;
                if (type == TetrominoType::O) evalScore += 20.f;
            }
            if (a.bumpiness > 6) {
                if (type == TetrominoType::T) evalScore += 25.f;
                if (type == TetrominoType::S) evalScore += 15.f;
                if (type == TetrominoType::Z) evalScore += 15.f;
            }
            for (int c = 0; c < BOARD_COLS; ++c) {
                if (a.colHeights[c] > a.averageHeight + 4.f) {
                    if (type == TetrominoType::I && rotation % 2 == 1) evalScore += 35.f;
                    if (type == TetrominoType::J || type == TetrominoType::L) evalScore += 15.f;
                }
            }
            int emptyWidth = a.emptiest_col_end - a.emptiest_col_start + 1;
            if (pieceWidth <= emptyWidth) evalScore += 20.f;
            if (a.holes > 3 && pieceWidth >= 3) evalScore -= 10.f;
            if (a.averageHeight > BOARD_ROWS * 0.6f) {
                evalScore += (4 - pieceWidth) * 10.f;
                evalScore += (4 - pieceHeight) * 5.f;
            }
            return evalScore;
        }

        Piece spawnPieceAware(float quality) {
            if (quality <= 0.f) return spawnPieceRandom();
            BoardAnalysis analysis = analyzeBoard();
            TetrominoType bestType = TetrominoType::I;
            int bestRot = 0;
            float bestScore = -1.f;
            int typeCount = static_cast<int>(TetrominoType::COUNT);
            for (int t = 0; t < typeCount; ++t) {
                for (int r = 0; r < 4; ++r) {
                    float s = scorePiece(static_cast<TetrominoType>(t), r, analysis);
                    if (s > bestScore) {
                        bestScore = s;
                        bestType = static_cast<TetrominoType>(t);
                        bestRot = r;
                    }
                }
            }
            std::uniform_real_distribution<float> chance(0.f, 1.f);
            Piece result;
            if (chance(rng) < quality) {
                result.type = bestType;
                result.rotation = bestRot;
            } else {
                result.type = randomType(rng);
                std::uniform_int_distribution<int> rotDist(0, 3);
                result.rotation = rotDist(rng);
            }
            result.col = BOARD_COLS / 2 - 2;
            result.row = 0;
            return result;
        }

        Piece spawnPieceRandom() {
            Piece p;
            p.type = randomType(rng);
            std::uniform_int_distribution<int> rotDist(0, 3);
            p.rotation = rotDist(rng);
            p.col = BOARD_COLS / 2 - 2;
            p.row = 0;
            return p;
        }

        void refreshSelectionPieces() {
            for (int i = 0; i < SELECTION_SIZE; ++i)
                selectionPieces[i] = spawnPieceAware(slotQuality(i));
        }

        void refreshSelectionPiecesRandom() {
            for (int i = 0; i < SELECTION_SIZE; ++i)
                selectionPieces[i] = spawnPieceRandom();
        }

        int chooseFallingCol(const Piece &piece) {
            BoardAnalysis a = analyzeBoard();
            int minC = INT_MAX, maxC = INT_MIN;

            for (auto [r,c]: piece.localCells()) {
                minC = std::min(minC, c);
                maxC = std::max(maxC, c);
            }

            if (minC == INT_MAX) {
                minC = 0;
                maxC = 0;
            }

            int minAllowedCol = -minC;

            int maxAllowedCol = BOARD_COLS - 1 - maxC;

            int targetCol = std::clamp(a.emptiest_col_start - minC, minAllowedCol, maxAllowedCol);

            std::uniform_real_distribution<float> chance(0.f, 1.f);
            if (chance(rng) < 0.7f) {
                std::uniform_int_distribution<int> jitter(-1, 1);
                // Hedefe yakın rastgele bir sapma eklerken de yeni sınırları kullanıyoruz
                return std::clamp(targetCol + jitter(rng), minAllowedCol, maxAllowedCol);
            }

            // 3. ADIM: Tamamen rastgele seçim yapılacaksa, sadece güvenli sınırlar içinde zarlıyoruz
            std::uniform_int_distribution<int> colDist(minAllowedCol, maxAllowedCol);
            return colDist(rng);
        }

        void stepFallingPiece() {
            Piece next = fallingPiece;
            next.row++;
            if (board.canPlace(next)) {
                fallingPiece = next;
            } else {
                board.place(fallingPiece);
                if (onPieceLocked) onPieceLocked();
                processLineClears();
                spawnFallingPiece();
            }
        }



        void processLineClears() {
            std::vector<int> clearedRows;
            for (int r = 0; r < BOARD_ROWS; ++r)
                if (board.isRowFull(r)) clearedRows.push_back(r);

            // Jeton kontrolü: jeton taşıyan parça bu satırlarda mıydı? (sadece Ascension modunda değilse)
            if (gameMode != GameMode::Ascension && fallingHasToken && !clearedRows.empty()) {
                slotMachine.addToken(1);
                fallingHasToken = false;
            }

            int n = board.clearFullRows();
            if (n > 0) {
                int pts = LINE_SCORES[std::min(n, 4)] * level;
                score += pts;
                linesCleared += n;

                // Her 1000 puanın katında 1 jeton ver (sadece Ascension modunda değilse)
                if (gameMode != GameMode::Ascension) {
                    int newMilestone = (score / 1000);
                    if (newMilestone > scoreTokenMilestone) {
                        int tokensToAdd = newMilestone - scoreTokenMilestone;
                        scoreTokenMilestone = newMilestone;
                        for (int t = 0; t < tokensToAdd; ++t) slotMachine.addToken(1);
                    }
                }

                recalcLevel();
                if (onLinesCleared) onLinesCleared(n, clearedRows);
            }
        }
    };
} // namespace tetris
