#include "MainMenuScene.hpp"
#include "tetris/TetrisConstants.hpp"
#include "tetris/ColorPalette.hpp"
#include <iostream>

namespace tetris {
    MainMenuScene::MainMenuScene(sf::RenderWindow &win, AppSettings *appSettings,
                                 std::function<void()> startCallback,
                                 std::function<void()> quitCallback)
        : window(win), settings(appSettings), onStartGame(startCallback), onQuitGame(quitCallback) {
        setupLayout();
    }

    void MainMenuScene::setupLayout() {
        float cx = 700.f / 2.f;

        // Palet butonları 2x2 grid (Pause menüsündeki gibi geniş)
        paletteBtns.clear();
        float startX = cx - 120.f;
        for(int i = 0; i < 4; ++i) {
            int col = i % 2;
            int row = i / 2;
            paletteBtns.push_back(sf::FloatRect({startX + col * 130.f, 170.f + row * 50.f}, {110.f, 40.f}));
        }

        // Butonları palet gridinin altına ittirdik
        btnSfx        = sf::FloatRect({cx - 120.f, 290.f}, {110.f, 40.f});
        btnMusic      = sf::FloatRect({cx + 10.f,  290.f}, {110.f, 40.f});

        btnNewGame    = sf::FloatRect({cx - 100.f, 360.f}, {200.f, 50.f});
        btnHighScores = sf::FloatRect({cx - 100.f, 430.f}, {200.f, 50.f});
        btnQuit       = sf::FloatRect({cx - 100.f, 500.f}, {200.f, 50.f});

        btnBack = sf::FloatRect({cx - 75.f, 600.f}, {150.f, 40.f});
    }

    void MainMenuScene::onEnter() {
        if (!font.openFromFile("assets/font.ttf")) {
            std::cerr << "[MainMenu] Font yuklenemedi!\n";
        } else {
            fontLoaded = true;
        }

        // Skorları dosyadan yükle
        highScores = ScoreManager::load("assets/scores.csv");
    }

    void MainMenuScene::onExit() {
    }

    void MainMenuScene::handleEvent(const sf::Event &event) {
        sf::Vector2f mouse = sf::Vector2f(sf::Mouse::getPosition(window));
        // Viewport koordinatlarına çevir
        mouse = window.mapPixelToCoords(sf::Mouse::getPosition(window));

        if (showScoreTable) {
            if (event.is<sf::Event::MouseMoved>()) {
                backHovered = btnBack.contains(mouse);
            } else if (const auto *e = event.getIf<sf::Event::MouseButtonPressed>()) {
                if (e->button == sf::Mouse::Button::Left && btnBack.contains(mouse)) {
                    showScoreTable = false;
                }
            }
            return; // Overlay açıksa ana menü eventlerini yut
        }

        if (event.is<sf::Event::MouseMoved>()) {
            hoveredBtn = -1;
            if (btnNewGame.contains(mouse)) hoveredBtn = 0;
            else if (btnHighScores.contains(mouse)) hoveredBtn = 1;
            else if (btnQuit.contains(mouse)) hoveredBtn = 2;
            else if (btnSfx.contains(mouse)) hoveredBtn = 3;
            else if (btnMusic.contains(mouse)) hoveredBtn = 4;
            else {
                for (int i = 0; i < 4; ++i) {
                    if (paletteBtns[i].contains(mouse)) hoveredBtn = 5 + i;
                }
            }
        } else if (const auto *e = event.getIf<sf::Event::MouseButtonPressed>()) {
            if (e->button == sf::Mouse::Button::Left) {
                if (btnNewGame.contains(mouse)) {
                    if (onStartGame) onStartGame();
                } else if (btnHighScores.contains(mouse)) {
                    showScoreTable = true;
                } else if (btnQuit.contains(mouse)) {
                    if (onQuitGame) onQuitGame();
                } else if (btnSfx.contains(mouse)) {
                    settings->sfxEnabled = !settings->sfxEnabled;
                } else if (btnMusic.contains(mouse)) {
                    settings->musicEnabled = !settings->musicEnabled;
                } else {
                    for (int i = 0; i < 4; ++i) {
                        if (paletteBtns[i].contains(mouse)) {
                            settings->activePalette = i;
                        }
                    }
                }
            }
        }
    }

    void MainMenuScene::update(float dt, engine::InputManager &input) {
        // Menüde hareket eden bir şey varsa buraya eklenebilir
    }

    void MainMenuScene::render(sf::RenderWindow &renderWindow, float /*alpha*/) {
        if (!fontLoaded) return;
        const auto &pal = PALETTES[settings->activePalette];

        sf::Color letterboxCol = pal.panelBorder;
        letterboxCol.r /= 2;
        letterboxCol.g /= 2;
        letterboxCol.b /= 2;
        renderWindow.clear(letterboxCol);

        sf::RectangleShape bgRect({700.f, 740.f});
        bgRect.setFillColor(pal.background);
        renderWindow.draw(bgRect);

        sf::Color gridCol = pal.gridLines;
        for (float x = 0; x <= 700.f; x += 35.f) {
            sf::RectangleShape line({1.f, 740.f});
            line.setPosition({x, 0.f});
            line.setFillColor(gridCol);
            renderWindow.draw(line);
        }
        for (float y = 0; y <= 740.f; y += 35.f) {
            sf::RectangleShape line({700.f, 1.f});
            line.setPosition({0.f, y});
            line.setFillColor(gridCol);
            renderWindow.draw(line);
        }

        auto drawText = [&](const std::string &str, float x, float y, int size, sf::Color col, bool center = false) {
            sf::Text t(font, str, size);
            t.setFillColor(col);
            if (center) {
                sf::FloatRect b = t.getLocalBounds();
                // SFML 3 stili merkezleme
                t.setOrigin({b.position.x + b.size.x / 2.f, b.position.y + b.size.y / 2.f});
                t.setPosition({x, y});
            } else {
                t.setPosition({x, y});
            }
            renderWindow.draw(t);
        };

        auto drawBtn = [&](sf::FloatRect rect, const std::string &text, bool isHovered) {
            sf::RectangleShape rs(rect.size);
            rs.setPosition(rect.position);
            rs.setFillColor(isHovered ? pal.uiAccent : pal.panelBorder);
            rs.setOutlineThickness(2.f);
            rs.setOutlineColor(pal.uiAccent);
            renderWindow.draw(rs);
            sf::Color txtCol = isHovered ? pal.background : sf::Color::White;
            drawText(text, rect.position.x + rect.size.x / 2.f, rect.position.y + rect.size.y / 2.f, 20, txtCol, true);
        };

        drawText("TETRIS", 354.f, 54.f, 60, pal.panelBorder, true);
        drawText("TETRIS", 350.f, 50.f, 60, pal.uiAccent, true);
        drawText("REFLEX", 350.f, 100.f, 40, pal.pieceI, true);

        drawText("COLOR PALETTE", 350.f, 150.f, 16, sf::Color::White, true);

        // YENİ: Detaylı Palet Çizimi
        for (int i = 0; i < 4; ++i) {
            const auto &tp = PALETTES[i];
            bool isActive = (settings->activePalette == i);
            bool isHovered = (hoveredBtn == 5 + i);

            sf::RectangleShape rs(paletteBtns[i].size);
            rs.setPosition(paletteBtns[i].position);
            rs.setFillColor(isHovered
                                ? sf::Color(std::min(255, tp.panelBg.r + 30), std::min(255, tp.panelBg.g + 30),
                                            std::min(255, tp.panelBg.b + 30))
                                : tp.panelBg);
            rs.setOutlineThickness(isActive ? 2.f : 1.f);
            rs.setOutlineColor(isActive ? pal.uiAccent : pal.panelBorder);
            renderWindow.draw(rs);

            sf::RectangleShape strip(sf::Vector2f{10.f, paletteBtns[i].size.y - 4.f});
            strip.setPosition({paletteBtns[i].position.x + 2.f, paletteBtns[i].position.y + 2.f});
            strip.setFillColor(tp.background);
            renderWindow.draw(strip);

            strip.setPosition({paletteBtns[i].position.x + 12.f, paletteBtns[i].position.y + 2.f});
            strip.setFillColor(tp.pieceI);
            renderWindow.draw(strip);

            strip.setPosition({paletteBtns[i].position.x + 22.f, paletteBtns[i].position.y + 2.f});
            strip.setFillColor(tp.pieceT);
            renderWindow.draw(strip);

            // Palet İsmi
            drawText(tp.name.substr(0, 8) + ".", paletteBtns[i].position.x + 65.f,
                     paletteBtns[i].position.y + paletteBtns[i].size.y / 2.f, 12, tp.uiText, true);
        }

        std::string sfxText = settings->sfxEnabled ? "SFX: ON" : "SFX: OFF";
        std::string musText = settings->musicEnabled ? "MUS: ON" : "MUS: OFF";
        drawBtn(btnSfx, sfxText, hoveredBtn == 3);
        drawBtn(btnMusic, musText, hoveredBtn == 4);

        drawBtn(btnNewGame, "NEW GAME", hoveredBtn == 0);
        drawBtn(btnHighScores, "HIGH SCORES", hoveredBtn == 1);
        drawBtn(btnQuit, "QUIT", hoveredBtn == 2);

        drawText("F11 - Toggle Fullscreen", 350.f, 680.f, 14, pal.uiAccent, true);

        if (showScoreTable) {
            renderScoreTable(renderWindow);
        }
    }

    void MainMenuScene::renderScoreTable(sf::RenderWindow &renderWindow) {
        sf::RectangleShape dim({700.f, 740.f});
        // Arka planı neredeyse tam siyah yaptık ki arkadaki menü okumayı zorlaştırmasın
        dim.setFillColor(sf::Color(0, 0, 0, 245));
        renderWindow.draw(dim);

        auto drawText = [&](const std::string &str, float x, float y, int size, sf::Color col, bool center = false) {
            sf::Text t(font, str, size);
            t.setFillColor(col);
            if (center) {
                sf::FloatRect b = t.getLocalBounds();
                t.setOrigin({b.position.x + b.size.x / 2.f, b.position.y + b.size.y / 2.f});
                t.setPosition({x, y});
            } else {
                t.setPosition({x, y});
            }
            renderWindow.draw(t);
        };

        drawText("HIGH SCORES", 350.f, 100.f, 40, PALETTES[settings->activePalette].uiAccent, true);

        float startY = 190.f;

        // Sütunların sabit X koordinatları
        float cRank = 150.f, cName = 250.f, cScore = 380.f, cLvl = 480.f, cLines = 560.f;

        // Başlıklar
        drawText("RANK", cRank, startY, 20, sf::Color::Yellow, true);
        drawText("NAME", cName, startY, 20, sf::Color::Yellow, true);
        drawText("SCORE", cScore, startY, 20, sf::Color::Yellow, true);
        drawText("LVL", cLvl, startY, 20, sf::Color::Yellow, true);
        drawText("LINES", cLines, startY, 20, sf::Color::Yellow, true);

        // Veriler
        for (size_t i = 0; i < highScores.size(); ++i) {
            float rowY = startY + 50.f + (i * 35.f);

            drawText(std::to_string(i + 1), cRank, rowY, 18, sf::Color::White, true);
            drawText(highScores[i].name, cName, rowY, 18, sf::Color::White, true);
            drawText(std::to_string(highScores[i].score), cScore, rowY, 18, sf::Color::White, true);
            drawText(std::to_string(highScores[i].level), cLvl, rowY, 18, sf::Color::White, true);
            drawText(std::to_string(highScores[i].lines), cLines, rowY, 18, sf::Color::White, true);
        }

        if (highScores.empty()) {
            drawText("No scores saved yet.", 350.f, startY + 80.f, 18, sf::Color(150, 150, 150), true);
        }

        sf::RectangleShape rs(btnBack.size);
        rs.setPosition(btnBack.position);
        rs.setFillColor(backHovered
                            ? PALETTES[settings->activePalette].uiAccent
                            : PALETTES[settings->activePalette].panelBorder);
        rs.setOutlineThickness(2.f);
        rs.setOutlineColor(PALETTES[settings->activePalette].uiAccent);
        renderWindow.draw(rs);

        sf::Color txtCol = backHovered ? PALETTES[settings->activePalette].background : sf::Color::White;
        drawText("BACK", btnBack.position.x + btnBack.size.x / 2.f, btnBack.position.y + btnBack.size.y / 2.f, 20,
                 txtCol, true);
    }
} // namespace tetris//
// Created by e-r-e on 5/20/2026.
//
