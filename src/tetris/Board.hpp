#pragma once

// ─────────────────────────────────────────────────────────────────────────────
// Board.hpp  —  Oyun tahtası: grid durumu, çarpışma, satır temizleme
//
// Board koordinat sistemi:
//   col 0..COLS-1  →  x (sol = 0)
//   row 0..ROWS-1  →  y (üst = 0)
//
// Hücre değerleri:
//   0              = boş
//   1-7            = yerleşmiş blok rengi (TetrominoType+1)
//   FALLING_MARK   = düşen parçanın geçici izi (çizim amaçlı)
// ─────────────────────────────────────────────────────────────────────────────

#include "Tetromino.hpp"
#include <limits>    // std::numeric_limits
#include <array>
#include <vector>
#include <algorithm> // std::min, std::max, std::clamp
#include <SFML/Graphics/Color.hpp>
#include <functional>

namespace tetris {

enum class SpecialTileType {
    None,
    Counter,
    Bomb
};

struct SpecialTile {
    SpecialTileType type{SpecialTileType::None};
    int counterValue{0};
};

inline constexpr int BOARD_COLS  = 10;
inline constexpr int BOARD_ROWS  = 18;
inline constexpr int CELL_SIZE   = 32;   // Piksel cinsinden hücre boyutu

// Hücrede saklanan rengi temsil eder
using CellColor = sf::Color;
inline const CellColor EMPTY_COLOR = sf::Color::Transparent;

// ─────────────────────────────────────────────────────────────────────────────
// Board
// ─────────────────────────────────────────────────────────────────────────────
class Board {
public:
    Board() { clear(); }

    std::function<void(int col, int row, sf::Color color)> onCellCleared;
    std::function<void(int col, int row)> onBombExploded;

    void clear() {
        for (auto& row : grid)
            row.fill(EMPTY_COLOR);
        for (auto& row : originalGarbage)
            row.fill(false);
        for (auto& row : specialTiles)
            row.fill({SpecialTileType::None, 0});
    }

    // ── Hücre erişimi ────────────────────────────────────────────────────────
    bool isEmpty(int col, int row) const {
        if (!inBounds(col, row)) return false;
        return grid[row][col] == EMPTY_COLOR;
    }

    bool inBounds(int col, int row) const {
        return col >= 0 && col < BOARD_COLS && row >= 0 && row < BOARD_ROWS;
    }

    CellColor getCell(int col, int row) const {
        if (!inBounds(col, row)) return EMPTY_COLOR;
        return grid[row][col];
    }

    void setCell(int col, int row, CellColor color) {
        if (inBounds(col, row)) {
            if (color == EMPTY_COLOR && specialTiles[row][col].type == SpecialTileType::Bomb) {
                specialTiles[row][col].type = SpecialTileType::None;
                triggerExplosion(col, row);
            }
            
            if (color == EMPTY_COLOR && grid[row][col] != EMPTY_COLOR) {
                if (onCellCleared) {
                    onCellCleared(col, row, grid[row][col]);
                }
            }

            grid[row][col] = color;
            if (color == EMPTY_COLOR) {
                originalGarbage[row][col] = false;
                specialTiles[row][col] = {SpecialTileType::None, 0};
            }
        }
    }

    void setOriginalGarbage(int col, int row, bool val) {
        if (inBounds(col, row)) originalGarbage[row][col] = val;
    }

    bool isOriginalGarbage(int col, int row) const {
        if (!inBounds(col, row)) return false;
        return originalGarbage[row][col];
    }

    int countOriginalGarbage() const {
        int count = 0;
        for (int r = 0; r < BOARD_ROWS; ++r) {
            for (int c = 0; c < BOARD_COLS; ++c) {
                if (originalGarbage[r][c] && grid[r][c] != EMPTY_COLOR) {
                    count++;
                }
            }
        }
        return count;
    }

    // ── Çarpışma kontrolü ────────────────────────────────────────────────────
    bool canPlace(const Piece& piece, const Piece* fallingPiece = nullptr) const {

        // 1. Eğer düşen parça gönderildiyse, sürüklenen parça ile çakışıyor mu kontrol et
        if (fallingPiece != nullptr) {
            auto draggedCells = piece.cells();
            auto fallingCells = fallingPiece->cells();

            for (const auto& dCell : draggedCells) {
                for (const auto& fCell : fallingCells) {
                    if (dCell == fCell) {
                        return false; // Düşen parça ile çakışma var!
                    }
                }
            }
        }

        // 2. Tahta sınırları ve yerleşmiş bloklarla çarpışma kontrolü
        for (auto [r, c] : piece.cells()) {
            if (c < 0 || c >= BOARD_COLS) return false;
            if (r >= BOARD_ROWS)          return false;
            if (r < 0)                    continue; // Üst sınırın üstü geçerli
            if (!isEmpty(c, r))           return false;
        }
        return true;
    }

    void place(const Piece& piece) {
        for (auto [r, c] : piece.cells()) {
            if (inBounds(c, r)) {
                grid[r][c] = piece.color();
            }
        }
    }

    // ── Satır temizleme ──────────────────────────────────────────────────────
    int clearFullRows() {
        int cleared = 0;
        for (int row = BOARD_ROWS - 1; row >= 0; ) {
            if (isRowFull(row)) {
                removeRowCustom(row);
                ++cleared;
            } else {
                --row;
            }
        }
        return cleared;
    }

    void triggerExplosion(int centerCol, int centerRow) {
        std::vector<std::pair<int, int>> bombQueue;
        bombQueue.push_back({centerCol, centerRow});
        
        while (!bombQueue.empty()) {
            auto [currCol, currRow] = bombQueue.back();
            bombQueue.pop_back();
            
            if (inBounds(currCol, currRow)) {
                specialTiles[currRow][currCol].type = SpecialTileType::None;
                if (onBombExploded) onBombExploded(currCol, currRow);
            }

            for (int r = currRow - 1; r <= currRow + 1; ++r) {
                for (int c = currCol - 1; c <= currCol + 1; ++c) {
                    if (inBounds(c, r)) {
                        if (specialTiles[r][c].type == SpecialTileType::Bomb) {
                            bombQueue.push_back({c, r});
                        }
                        setCell(c, r, EMPTY_COLOR);
                    }
                }
            }
        }
    }

    void removeRowCustom(int row) {
        // Check and trigger bomb explosions in this row before any shift
        for (int c = 0; c < BOARD_COLS; ++c) {
            if (specialTiles[row][c].type == SpecialTileType::Bomb) {
                triggerExplosion(c, row);
            }
        }

        for (int c = 0; c < BOARD_COLS; ++c) {
            bool survives = false;
            if (specialTiles[row][c].type == SpecialTileType::Counter && specialTiles[row][c].counterValue > 1) {
                survives = true;
            }

            if (survives) {
                specialTiles[row][c].counterValue--;
            } else {
                for (int r = row; r > 0; --r) {
                    grid[r][c] = grid[r - 1][c];
                    originalGarbage[r][c] = originalGarbage[r - 1][c];
                    specialTiles[r][c] = specialTiles[r - 1][c];
                }
                grid[0][c] = EMPTY_COLOR;
                originalGarbage[0][c] = false;
                specialTiles[0][c] = {SpecialTileType::None, 0};
            }
        }
    }

    void removeRow(int row) {
        for (int r = row; r > 0; --r) {
            grid[r] = grid[r - 1];
            originalGarbage[r] = originalGarbage[r - 1];
            specialTiles[r] = specialTiles[r - 1];
        }
        grid[0].fill(EMPTY_COLOR);
        originalGarbage[0].fill(false);
        specialTiles[0].fill({SpecialTileType::None, 0});
    }

    void setSpecialTile(int col, int row, SpecialTile tile) {
        if (inBounds(col, row)) specialTiles[row][col] = tile;
    }

    SpecialTile getSpecialTile(int col, int row) const {
        if (!inBounds(col, row)) return {SpecialTileType::None, 0};
        return specialTiles[row][col];
    }

    bool isRowFull(int row) const {
        for (int c = 0; c < BOARD_COLS; ++c) {
            if (isEmpty(c, row)) return false;
        }
        return true;
    }

    bool isTopOut() const {
        for (int row = 0; row < 2; ++row) {
            for (int c = 0; c < BOARD_COLS; ++c) {
                if (!isEmpty(c, row)) return true;
            }
        }
        return false;
    }

    // ── "Ghost" çizgisi (Normal düşen parça için) ─────────────────────────────
    Piece ghostPiece(Piece piece) const {
        while (true) {
            Piece next = piece;
            next.row++;
            if (!canPlace(next)) break;
            piece = next;
        }
        return piece;
    }

    // ── Sürüklenen parça için "en iyi yerleşim" (Puzzle Tarzı Serbest Mod) ────
    std::pair<bool, Piece> findDropPosition(Piece piece, int targetCol, int targetRow, const Piece* fallingPiece = nullptr) const {
        int minC = std::numeric_limits<int>::max();
        int maxC = std::numeric_limits<int>::min();
        int minR = std::numeric_limits<int>::max();
        int maxR = std::numeric_limits<int>::min();

        for (auto [r, c] : piece.localCells()) {
            minC = std::min(minC, c);
            maxC = std::max(maxC, c);
            minR = std::min(minR, r);
            maxR = std::max(maxR, r);
        }
        if (minC == std::numeric_limits<int>::max()) {
            minC = 0; maxC = 0; minR = 0; maxR = 0;
        }

        int centerOffsetX = minC + (maxC - minC + 1) / 2;
        int centerOffsetY = minR + (maxR - minR + 1) / 2;

        piece.col = targetCol - centerOffsetX;
        piece.row = targetRow - centerOffsetY;

        piece.col = std::clamp(piece.col, -minC, BOARD_COLS - 1 - maxC);
        piece.row = std::clamp(piece.row, -maxR, BOARD_ROWS - 1 - maxR);

        if (!canPlace(piece, fallingPiece)) return {false, piece};

        return {true, piece};
    }

    // ── Direkt yerleşim: drag & drop bırakma anı (Puzzle Tarzı) ───────────────
    std::pair<bool, Piece> getDirectPlacement(Piece piece, int targetCol, int targetRow, const Piece* fallingPiece = nullptr) const {
        return findDropPosition(piece, targetCol, targetRow, fallingPiece);
    }

private:


    std::array<std::array<CellColor, BOARD_COLS>, BOARD_ROWS> grid;
    std::array<std::array<bool, BOARD_COLS>, BOARD_ROWS> originalGarbage;
    std::array<std::array<SpecialTile, BOARD_COLS>, BOARD_ROWS> specialTiles;
};

} // namespace tetris