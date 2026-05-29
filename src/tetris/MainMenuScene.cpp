#include "MainMenuScene.hpp"
#include "tetris/TetrisScene.hpp"
#include "tetris/TetrisConstants.hpp"
#include "tetris/ColorPalette.hpp"
#include "../WindowUtils.hpp"
#include <iostream>

namespace tetris {
    MainMenuScene::MainMenuScene(sf::RenderWindow &win, AppSettings *appSettings)
        : window(win), settings(appSettings) {
        setupLayout();
        btnScales.assign(30, 1.0f);
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

        // Mode selector buttons
        m_modeBtns.clear();
        float cardW = 180.f;
        float cardH = 110.f;

        // Row 0
        m_modeBtns.push_back(sf::FloatRect({40.f, 180.f}, {cardW, cardH}));
        m_modeBtns.push_back(sf::FloatRect({260.f, 180.f}, {cardW, cardH}));
        m_modeBtns.push_back(sf::FloatRect({480.f, 180.f}, {cardW, cardH}));

        // Row 1
        m_modeBtns.push_back(sf::FloatRect({40.f, 320.f}, {cardW, cardH}));
        m_modeBtns.push_back(sf::FloatRect({260.f, 320.f}, {cardW, cardH}));
        m_modeBtns.push_back(sf::FloatRect({480.f, 320.f}, {cardW, cardH}));

        // Row 2
        m_modeBtns.push_back(sf::FloatRect({135.f, 460.f}, {cardW, cardH}));
        m_modeBtns.push_back(sf::FloatRect({385.f, 460.f}, {cardW, cardH}));

        m_btnBackModeSelect = sf::FloatRect({275.f, 620.f}, {150.f, 40.f});

        // Custom Mode Configuration screen layout
        float col1X = cx - 240.f;
        float col2X = cx + 20.f;
        float btnW = 220.f;
        float btnH = 36.f;
        float spacingY = 40.f;
        float startY = 160.f;

        m_customUI.btnGravity = sf::FloatRect({col1X, startY}, {btnW, btnH});
        m_customUI.btnTactical = sf::FloatRect({col1X, startY + spacingY}, {btnW, btnH});
        m_customUI.btnSlot = sf::FloatRect({col1X, startY + spacingY * 2}, {btnW, btnH});
        m_customUI.btnAscension = sf::FloatRect({col1X, startY + spacingY * 3}, {btnW, btnH});

        m_customUI.btnGarbage = sf::FloatRect({col2X, startY}, {btnW, btnH});
        m_customUI.btnGarbageRatio = sf::FloatRect({col2X, startY + spacingY}, {btnW, btnH});
        m_customUI.btnSpecial = sf::FloatRect({col2X, startY + spacingY * 2}, {btnW, btnH});
        m_customUI.btnScore = sf::FloatRect({col2X, startY + spacingY * 3}, {btnW, btnH});

        m_customUI.btnTime = sf::FloatRect({cx - 150.f, startY + spacingY * 4.2f}, {300.f, btnH});

        m_customUI.btnStart = sf::FloatRect({cx - 120.f, 490.f}, {240.f, 45.f});
        m_customUI.btnBack = sf::FloatRect({cx - 120.f, 550.f}, {240.f, 40.f});

        // Score table navigation arrows
        btnScorePrev = sf::FloatRect({180.f, 105.f}, {40.f, 30.f});
        btnScoreNext = sf::FloatRect({480.f, 105.f}, {40.f, 30.f});

        // Garbage Ratio modal buttons
        btnRatioOpt1 = sf::FloatRect({170.f, 320.f}, {160.f, 100.f});
        btnRatioOpt2 = sf::FloatRect({370.f, 320.f}, {160.f, 100.f});
        btnRatioBack = sf::FloatRect({275.f, 520.f}, {150.f, 40.f});

        // Score Time modal buttons
        btnScoreTimeOpt1 = sf::FloatRect({47.5f, 320.f}, {170.f, 110.f});
        btnScoreTimeOpt2 = sf::FloatRect({265.f, 320.f}, {170.f, 110.f});
        btnScoreTimeOpt3 = sf::FloatRect({482.5f, 320.f}, {170.f, 110.f});
        btnScoreTimeBack = sf::FloatRect({275.f, 520.f}, {150.f, 40.f});
    }

    void MainMenuScene::onEnter() {
        if (!font.openFromFile("assets/font.ttf")) {
            std::cerr << "[MainMenu] Font yuklenemedi!\n";
        } else {
            fontLoaded = true;
        }

        m_scoreTableMode = settings->activeMode;
        m_gameStats = StatsManager::load("assets/stats.csv");
    }

    void MainMenuScene::onExit() {
    }

    static float easeOutBack(float t) {
        const float c1 = 1.70158f;
        const float c3 = c1 + 1.f;
        float tMinus1 = t - 1.f;
        return 1.f + c3 * std::pow(tMinus1, 3.f) + c1 * std::pow(tMinus1, 2.f);
    }

    void MainMenuScene::handleEvent(const sf::Event &event) {
        sf::Vector2f mouse = sf::Vector2f(sf::Mouse::getPosition(window));
        // Viewport koordinatlarına çevir
        mouse = window.mapPixelToCoords(sf::Mouse::getPosition(window));

        if (m_showCustomConfig) {
            if (event.is<sf::Event::MouseMoved>()) {
                hoveredCustomBtn = -1;
                if (m_customUI.btnGravity.contains(mouse)) hoveredCustomBtn = 0;
                else if (m_customUI.btnTactical.contains(mouse)) hoveredCustomBtn = 1;
                else if (m_customUI.btnSlot.contains(mouse)) hoveredCustomBtn = 2;
                else if (m_customUI.btnAscension.contains(mouse)) hoveredCustomBtn = 3;
                else if (m_customUI.btnGarbage.contains(mouse)) hoveredCustomBtn = 4;
                else if (m_customUI.btnGarbageRatio.contains(mouse)) {
                    if (settings->customSettings.garbageBoardEnabled) hoveredCustomBtn = 5;
                }
                else if (m_customUI.btnSpecial.contains(mouse)) hoveredCustomBtn = 6;
                else if (m_customUI.btnScore.contains(mouse)) hoveredCustomBtn = 7;
                else if (m_customUI.btnTime.contains(mouse)) hoveredCustomBtn = 8;
                else if (m_customUI.btnStart.contains(mouse)) hoveredCustomBtn = 9;
                else if (m_customUI.btnBack.contains(mouse)) hoveredCustomBtn = 10;
                
                // Option B: Hover Spark Trails
                if (hoveredCustomBtn != -1) {
                    const auto& pal = PALETTES[settings->activePalette];
                    for (int p = 0; p < 2; ++p) {
                        float vx = ((std::rand() % 100) - 50) * 1.5f;
                        float vy = ((std::rand() % 100) - 50) * 1.5f;
                        float lifetime = 0.3f + (std::rand() % 30) / 100.f;
                        float size = 2.f + (std::rand() % 3);
                        sf::Color pColor = pal.uiAccent;
                        pColor.a = 200;
                        m_particles.push_back({mouse, {vx, vy}, pColor, 0.f, lifetime, size});
                    }
                }
            } else if (const auto *e = event.getIf<sf::Event::MouseButtonPressed>()) {
                if (e->button == sf::Mouse::Button::Left) {
                    auto& cfg = settings->customSettings;
                    if (m_customUI.btnGravity.contains(mouse)) {
                        cfg.gravityEnabled = !cfg.gravityEnabled;
                        if (cfg.gravityEnabled) {
                            cfg.tacticalStepEnabled = false;
                        }
                    } else if (m_customUI.btnTactical.contains(mouse)) {
                        cfg.tacticalStepEnabled = !cfg.tacticalStepEnabled;
                        if (cfg.tacticalStepEnabled) {
                            cfg.gravityEnabled = false;
                        }
                    } else if (m_customUI.btnSlot.contains(mouse)) {
                        cfg.slotMachineEnabled = !cfg.slotMachineEnabled;
                    } else if (m_customUI.btnAscension.contains(mouse)) {
                        cfg.ascensionEnabled = !cfg.ascensionEnabled;
                    } else if (m_customUI.btnGarbage.contains(mouse)) {
                        cfg.garbageBoardEnabled = !cfg.garbageBoardEnabled;
                    } else if (m_customUI.btnGarbageRatio.contains(mouse) && cfg.garbageBoardEnabled) {
                        if (cfg.garbageRatio == 0.30f) cfg.garbageRatio = 0.40f;
                        else if (cfg.garbageRatio == 0.40f) cfg.garbageRatio = 0.60f;
                        else if (cfg.garbageRatio == 0.60f) cfg.garbageRatio = 0.70f;
                        else cfg.garbageRatio = 0.30f;
                    } else if (m_customUI.btnSpecial.contains(mouse)) {
                        cfg.specialBlocksEnabled = !cfg.specialBlocksEnabled;
                    } else if (m_customUI.btnScore.contains(mouse)) {
                        if (!cfg.targetScoreEnabled) {
                            cfg.targetScoreEnabled = true;
                            cfg.targetScore = 1000;
                        } else {
                            if (cfg.targetScore == 1000) cfg.targetScore = 2000;
                            else if (cfg.targetScore == 2000) cfg.targetScore = 3000;
                            else if (cfg.targetScore == 3000) cfg.targetScore = 5000;
                            else if (cfg.targetScore == 5000) cfg.targetScore = 10000;
                            else cfg.targetScoreEnabled = false;
                        }
                    } else if (m_customUI.btnTime.contains(mouse)) {
                        if (!cfg.timeLimitEnabled) {
                            cfg.timeLimitEnabled = true;
                            cfg.timeLimit = 60.f;
                        } else {
                            if (cfg.timeLimit == 60.f) cfg.timeLimit = 90.f;
                            else if (cfg.timeLimit == 90.f) cfg.timeLimit = 120.f;
                            else if (cfg.timeLimit == 120.f) cfg.timeLimit = 180.f;
                            else if (cfg.timeLimit == 180.f) cfg.timeLimit = 300.f;
                            else cfg.timeLimitEnabled = false;
                        }
                    } else if (m_customUI.btnStart.contains(mouse)) {
                        settings->activeMode = GameMode::Custom;
                        m_sceneManager->replace(std::make_unique<tetris::TetrisScene>(window, settings));
                    } else if (m_customUI.btnBack.contains(mouse)) {
                        m_showCustomConfig = false;
                        m_showModeSelect = true;
                        m_introTimer = 0.f;
                        hoveredBtn = -1;
                    }
                }
            }
            return;
        }

        if (m_showGarbageSelect) {
            if (event.is<sf::Event::MouseMoved>()) {
                hoveredRatioBtn = -1;
                if (btnRatioOpt1.contains(mouse)) hoveredRatioBtn = 0;
                else if (btnRatioOpt2.contains(mouse)) hoveredRatioBtn = 1;
                else if (btnRatioBack.contains(mouse)) hoveredRatioBtn = 2;
            } else if (const auto *e = event.getIf<sf::Event::MouseButtonPressed>()) {
                if (e->button == sf::Mouse::Button::Left) {
                    if (btnRatioOpt1.contains(mouse)) {
                        if (m_pendingMode == GameMode::TacticalGarbageClear) {
                            settings->garbageRatio = 0.60f;
                        } else {
                            settings->garbageRatio = 0.30f;
                        }
                        settings->activeMode = m_pendingMode;
                        m_sceneManager->replace(std::make_unique<tetris::TetrisScene>(window, settings));
                    } else if (btnRatioOpt2.contains(mouse)) {
                        if (m_pendingMode == GameMode::TacticalGarbageClear) {
                            settings->garbageRatio = 0.70f;
                        } else {
                            settings->garbageRatio = 0.40f;
                        }
                        settings->activeMode = m_pendingMode;
                        m_sceneManager->replace(std::make_unique<tetris::TetrisScene>(window, settings));
                    } else if (btnRatioBack.contains(mouse)) {
                        m_showGarbageSelect = false;
                        hoveredRatioBtn = -1;
                    }
                }
            }
            return;
        }

        if (m_showScoreTimeSelect) {
            if (event.is<sf::Event::MouseMoved>()) {
                hoveredScoreTimeBtn = -1;
                if (btnScoreTimeOpt1.contains(mouse)) hoveredScoreTimeBtn = 0;
                else if (btnScoreTimeOpt2.contains(mouse)) hoveredScoreTimeBtn = 1;
                else if (btnScoreTimeOpt3.contains(mouse)) hoveredScoreTimeBtn = 2;
                else if (btnScoreTimeBack.contains(mouse)) hoveredScoreTimeBtn = 3;
            } else if (const auto *e = event.getIf<sf::Event::MouseButtonPressed>()) {
                if (e->button == sf::Mouse::Button::Left) {
                    if (btnScoreTimeOpt1.contains(mouse)) {
                        settings->scoreTimeTarget = 1000;
                        settings->scoreTimeLimit = 90.f;
                        settings->activeMode = GameMode::ScoreTimeAttack;
                        m_sceneManager->replace(std::make_unique<tetris::TetrisScene>(window, settings));
                    } else if (btnScoreTimeOpt2.contains(mouse)) {
                        settings->scoreTimeTarget = 2000;
                        settings->scoreTimeLimit = 120.f;
                        settings->activeMode = GameMode::ScoreTimeAttack;
                        m_sceneManager->replace(std::make_unique<tetris::TetrisScene>(window, settings));
                    } else if (btnScoreTimeOpt3.contains(mouse)) {
                        settings->scoreTimeTarget = 5000;
                        settings->scoreTimeLimit = 180.f;
                        settings->activeMode = GameMode::ScoreTimeAttack;
                        m_sceneManager->replace(std::make_unique<tetris::TetrisScene>(window, settings));
                    } else if (btnScoreTimeBack.contains(mouse)) {
                        m_showScoreTimeSelect = false;
                        hoveredScoreTimeBtn = -1;
                    }
                }
            }
            return;
        }

        if (showScoreTable) {
            if (event.is<sf::Event::MouseMoved>()) {
                backHovered = btnBack.contains(mouse);
                prevScoreHovered = btnScorePrev.contains(mouse);
                nextScoreHovered = btnScoreNext.contains(mouse);
            } else if (const auto *e = event.getIf<sf::Event::MouseButtonPressed>()) {
                if (e->button == sf::Mouse::Button::Left) {
                    if (btnBack.contains(mouse)) {
                        showScoreTable = false;
                        m_introTimer = 0.f; // Reset intro so buttons slide back in!
                    } else if (btnScorePrev.contains(mouse)) {
                        int m = static_cast<int>(m_scoreTableMode) - 1;
                        if (m < 0) m = 7;
                        m_scoreTableMode = static_cast<GameMode>(m);
                    } else if (btnScoreNext.contains(mouse)) {
                        int m = static_cast<int>(m_scoreTableMode) + 1;
                        if (m > 7) m = 0;
                        m_scoreTableMode = static_cast<GameMode>(m);
                    }
                }
            }
            return; // Overlay açıksa ana menü eventlerini yut
        }

        if (m_showModeSelect) {
            if (event.is<sf::Event::MouseMoved>()) {
                hoveredBtn = -1;
                for (int i = 0; i < 8; ++i) {
                    if (m_modeBtns[i].contains(mouse)) {
                        hoveredBtn = 10 + i;
                    }
                }
                if (m_btnBackModeSelect.contains(mouse)) {
                    hoveredBtn = 18;
                }

                // Hover Spark Trails for mode selection
                if (hoveredBtn != -1) {
                    const auto& pal = PALETTES[settings->activePalette];
                    for (int p = 0; p < 2; ++p) {
                        float vx = ((std::rand() % 100) - 50) * 1.5f;
                        float vy = ((std::rand() % 100) - 50) * 1.5f;
                        float lifetime = 0.3f + (std::rand() % 30) / 100.f;
                        float size = 2.f + (std::rand() % 3);
                        sf::Color pColor = pal.uiAccent;
                        pColor.a = 200;
                        m_particles.push_back({mouse, {vx, vy}, pColor, 0.f, lifetime, size});
                    }
                }
            } else if (const auto *e = event.getIf<sf::Event::MouseButtonPressed>()) {
                if (e->button == sf::Mouse::Button::Left) {
                    for (int i = 0; i < 8; ++i) {
                        if (m_modeBtns[i].contains(mouse)) {
                            m_clickedBtn = 10 + i;
                            m_clickTimer = 0.f;
                            GameMode mode = static_cast<GameMode>(i);
                            if (mode == GameMode::GarbageClear || mode == GameMode::GarbageTimeAttack || mode == GameMode::TacticalGarbageClear) {
                                m_showGarbageSelect = true;
                                m_pendingMode = mode;
                                hoveredRatioBtn = -1;
                            } else if (mode == GameMode::ScoreTimeAttack) {
                                m_showScoreTimeSelect = true;
                                hoveredScoreTimeBtn = -1;
                            } else if (mode == GameMode::Custom) {
                                // Transition to custom config will occur in update() after clicked animation
                            } else {
                                settings->activeMode = mode;
                                m_sceneManager->replace(std::make_unique<tetris::TetrisScene>(window, settings));
                            }
                            return;
                        }
                    }
                    if (m_btnBackModeSelect.contains(mouse)) {
                        m_clickedBtn = 18;
                        m_clickTimer = 0.f;
                    }
                }
            }
            return;
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

            // Option B: Hover Spark Trails
            if (hoveredBtn != -1) {
                const auto& pal = PALETTES[settings->activePalette];
                for (int p = 0; p < 2; ++p) {
                    float vx = ((std::rand() % 100) - 50) * 1.5f;
                    float vy = ((std::rand() % 100) - 50) * 1.5f;
                    float lifetime = 0.3f + (std::rand() % 30) / 100.f;
                    float size = 2.f + (std::rand() % 3);
                    sf::Color pColor = pal.uiAccent;
                    pColor.a = 200;
                    m_particles.push_back({mouse, {vx, vy}, pColor, 0.f, lifetime, size});
                }
            }
        } else if (const auto *e = event.getIf<sf::Event::MouseButtonPressed>()) {
            if (e->button == sf::Mouse::Button::Left) {
                if (btnNewGame.contains(mouse)) {
                    m_clickedBtn = 0;
                    m_clickTimer = 0.f;
                } else if (btnHighScores.contains(mouse)) {
                    m_clickedBtn = 1;
                    m_clickTimer = 0.f;
                } else if (btnQuit.contains(mouse)) {
                    m_clickedBtn = 2;
                    m_clickTimer = 0.f;
                } else if (btnSfx.contains(mouse)) {
                    m_clickedBtn = 3;
                    m_clickTimer = 0.f;
                    settings->sfxEnabled = !settings->sfxEnabled;
                } else if (btnMusic.contains(mouse)) {
                    m_clickedBtn = 4;
                    m_clickTimer = 0.f;
                    settings->musicEnabled = !settings->musicEnabled;
                } else {
                    for (int i = 0; i < 4; ++i) {
                        if (paletteBtns[i].contains(mouse)) {
                            m_clickedBtn = 5 + i;
                            m_clickTimer = 0.f;
                            settings->activePalette = i;
                        }
                    }
                }
            }
        }
    }

    void MainMenuScene::update(float dt, engine::InputManager &/*input*/) {
        m_introTimer += dt;
        m_elapsedTime += dt;
        if (showScoreTable) {
            m_scoreTableTimer += dt;
        } else {
            m_scoreTableTimer = 0.f;
        }

        // Handle button click delays
        if (m_clickedBtn != -1) {
            m_clickTimer += dt;
            if (m_clickedBtn == 0 && m_clickTimer >= 0.2f && !m_showModeSelect) {
                m_showModeSelect = true;
                m_introTimer = 0.f;
                m_clickedBtn = -1;
            }
            if (m_clickedBtn == 1 && m_clickTimer >= 0.2f && !showScoreTable) {
                showScoreTable = true;
                m_scoreTableTimer = 0.f;
                m_clickedBtn = -1;
            }
            if (m_clickedBtn == 2 && m_clickTimer >= 0.25f) {
                window.close();
            }
            if (m_clickedBtn == 17 && m_clickTimer >= 0.2f) {
                m_showCustomConfig = true;
                m_showModeSelect = false;
                m_introTimer = 0.f;
                m_clickedBtn = -1;
            }
            if (m_clickedBtn == 18 && m_clickTimer >= 0.2f) {
                m_showModeSelect = false;
                m_introTimer = 0.f;
                m_clickedBtn = -1;
            }
            // Reset state toggles click anim after a short time
            if ((m_clickedBtn == 3 || m_clickedBtn == 4 || (m_clickedBtn >= 5 && m_clickedBtn < 10)) && m_clickTimer >= 0.25f) {
                m_clickedBtn = -1;
            }
        }

        // Option A: Mouse-Reactive Parallax / Tilt
        sf::Vector2f mouse = window.mapPixelToCoords(sf::Mouse::getPosition(window));
        sf::Vector2f center(700.f * 0.5f, 740.f * 0.5f);
        sf::Vector2f mouseDiff = mouse - center;
        sf::Vector2f targetTilt = { mouseDiff.x * 0.05f, mouseDiff.y * 0.05f };
        m_mouseTilt += (targetTilt - m_mouseTilt) * 10.f * dt;

        for (size_t i = 0; i < btnScales.size(); ++i) {
            float target = (hoveredBtn == static_cast<int>(i)) ? 1.08f : 1.0f;
            btnScales[i] += (target - btnScales[i]) * 15.f * dt;
        }

        // Option B: Particle update
        for (auto it = m_particles.begin(); it != m_particles.end(); ) {
            it->position += it->velocity * dt;
            it->age += dt;
            if (it->age >= it->lifetime) {
                it = m_particles.erase(it);
            } else {
                ++it;
            }
        }
    }

    void MainMenuScene::render(sf::RenderWindow &renderWindow, float /*alpha*/) {
        if (!fontLoaded) return;
        updateViewport(renderWindow, 700.f, 740.f);
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

        auto drawText = [&](const std::string &str, float x, float y, int size, sf::Color col, bool center = false, float yOffset = 0.f, float xOffset = 0.f) {
            sf::Text t(font, str, size);
            t.setFillColor(col);
            if (center) {
                sf::FloatRect b = t.getLocalBounds();
                t.setOrigin({b.position.x + b.size.x / 2.f, b.position.y + b.size.y / 2.f});
                t.setPosition({x + xOffset, y + yOffset});
            } else {
                t.setPosition({x + xOffset, y + yOffset});
            }
            renderWindow.draw(t);
        };

        if (m_showGarbageSelect) {
            sf::RectangleShape dim({700.f, 740.f});
            dim.setFillColor(sf::Color(0, 0, 0, 235));
            renderWindow.draw(dim);

            drawText("SELECT GARBAGE RATIO", 350.f, 150.f, 32, pal.uiAccent, true);
            drawText("Choose the board garbage fill density to start.", 350.f, 210.f, 14, sf::Color(180, 180, 200), true);

            std::string opt1Text, opt2Text, opt1Sub, opt2Sub;
            if (m_pendingMode == GameMode::TacticalGarbageClear) {
                opt1Text = "60%"; opt1Sub = "Normal Grid";
                opt2Text = "70%"; opt2Sub = "Tactical Grid";
            } else {
                opt1Text = "30%"; opt1Sub = "Normal Density";
                opt2Text = "40%"; opt2Sub = "Hard Density";
            }

            auto drawCard = [&](sf::FloatRect rect, const std::string& title, const std::string& sub, bool hover) {
                sf::RectangleShape rs(rect.size);
                rs.setPosition(rect.position);
                rs.setFillColor(hover ? pal.panelBg : pal.background);
                rs.setOutlineColor(hover ? pal.uiAccent : pal.panelBorder);
                rs.setOutlineThickness(hover ? 3.f : 1.5f);
                renderWindow.draw(rs);

                drawText(title, rect.position.x + rect.size.x / 2.f, rect.position.y + 30.f, 28, hover ? pal.uiAccent : sf::Color::White, true);
                drawText(sub, rect.position.x + rect.size.x / 2.f, rect.position.y + 65.f, 11, sf::Color(160, 160, 160), true);
            };

            drawCard(btnRatioOpt1, opt1Text, opt1Sub, hoveredRatioBtn == 0);
            drawCard(btnRatioOpt2, opt2Text, opt2Sub, hoveredRatioBtn == 1);

            // Back button
            sf::RectangleShape rsBack(btnRatioBack.size);
            rsBack.setPosition(btnRatioBack.position);
            rsBack.setFillColor(hoveredRatioBtn == 2 ? pal.uiAccent : pal.panelBorder);
            rsBack.setOutlineThickness(2.f);
            rsBack.setOutlineColor(pal.uiAccent);
            renderWindow.draw(rsBack);

            sf::Color txtCol = (hoveredRatioBtn == 2) ? pal.background : sf::Color::White;
            drawText("BACK", btnRatioBack.position.x + btnRatioBack.size.x / 2.f, btnRatioBack.position.y + btnRatioBack.size.y / 2.f, 18, txtCol, true);
            return;
        }

        if (m_showScoreTimeSelect) {
            sf::RectangleShape dim({700.f, 740.f});
            dim.setFillColor(sf::Color(0, 0, 0, 235));
            renderWindow.draw(dim);

            drawText("SELECT SCORE TIME GOAL", 350.f, 150.f, 32, pal.uiAccent, true);
            drawText("Choose the target score and time limit to start.", 350.f, 210.f, 14, sf::Color(180, 180, 200), true);

            auto drawCard = [&](sf::FloatRect rect, const std::string& title, const std::string& scoreStr, const std::string& timeStr, bool hover) {
                sf::RectangleShape rs(rect.size);
                rs.setPosition(rect.position);
                rs.setFillColor(hover ? pal.panelBg : pal.background);
                rs.setOutlineColor(hover ? pal.uiAccent : pal.panelBorder);
                rs.setOutlineThickness(hover ? 3.f : 1.5f);
                renderWindow.draw(rs);

                drawText(title, rect.position.x + rect.size.x / 2.f, rect.position.y + 18.f, 18, hover ? pal.uiAccent : sf::Color::White, true);
                drawText("Target: " + scoreStr, rect.position.x + rect.size.x / 2.f, rect.position.y + 50.f, 12, sf::Color(220, 220, 100), true);
                drawText("Time: " + timeStr, rect.position.x + rect.size.x / 2.f, rect.position.y + 75.f, 12, sf::Color(100, 220, 255), true);
            };

            drawCard(btnScoreTimeOpt1, "QUICK RACE", "1000", "90s", hoveredScoreTimeBtn == 0);
            drawCard(btnScoreTimeOpt2, "STANDARD", "2000", "120s", hoveredScoreTimeBtn == 1);
            drawCard(btnScoreTimeOpt3, "ENDURANCE", "5000", "180s", hoveredScoreTimeBtn == 2);

            // Back button
            sf::RectangleShape rsBack(btnScoreTimeBack.size);
            rsBack.setPosition(btnScoreTimeBack.position);
            rsBack.setFillColor(hoveredScoreTimeBtn == 3 ? pal.uiAccent : pal.panelBorder);
            rsBack.setOutlineThickness(2.f);
            rsBack.setOutlineColor(pal.uiAccent);
            renderWindow.draw(rsBack);

            sf::Color txtCol = (hoveredScoreTimeBtn == 3) ? pal.background : sf::Color::White;
            drawText("BACK", btnScoreTimeBack.position.x + btnScoreTimeBack.size.x / 2.f, btnScoreTimeBack.position.y + btnScoreTimeBack.size.y / 2.f, 18, txtCol, true);
            return;
        }

        if (m_showCustomConfig) {
            bgRect.setFillColor(pal.background);
            renderWindow.draw(bgRect);

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

            drawText("CUSTOM MODE SETUP", 350.f, 90.f, 36, pal.uiAccent, true, m_mouseTilt.y * 1.2f, m_mouseTilt.x * 1.2f);
            drawText("Configure your own set of gameplay rules.", 350.f, 130.f, 13, sf::Color(160, 160, 180), true);

            auto& cfg = settings->customSettings;

            auto drawToggleBtn = [&](sf::FloatRect rect, const std::string& label, const std::string& valStr, bool active, bool hover, bool grayedOut = false) {
                sf::RectangleShape rs(rect.size);
                rs.setPosition(rect.position);
                
                sf::Color fillCol = hover ? pal.panelBg : pal.background;
                if (grayedOut) {
                    fillCol = sf::Color(25, 25, 30);
                }
                rs.setFillColor(fillCol);

                sf::Color borderCol = hover ? pal.uiAccent : pal.panelBorder;
                if (grayedOut) {
                    borderCol = sf::Color(60, 60, 70);
                }
                rs.setOutlineColor(borderCol);
                rs.setOutlineThickness(hover ? 2.5f : 1.5f);
                renderWindow.draw(rs);

                // Glow effect if hovered and not grayed out
                if (hover && !grayedOut) {
                    for (int j = 0; j < 2; ++j) {
                        sf::RectangleShape glow(rect.size + sf::Vector2f{j * 2.f, j * 2.f});
                        glow.setOrigin(glow.getSize() * 0.5f);
                        glow.setPosition(rect.position + rect.size * 0.5f);
                        glow.setFillColor(sf::Color::Transparent);
                        sf::Color gc = pal.uiAccent;
                        gc.a = static_cast<std::uint8_t>(60 - j * 20);
                        glow.setOutlineColor(gc);
                        glow.setOutlineThickness(1.f);
                        renderWindow.draw(glow);
                    }
                }

                // Draw Label
                sf::Color lblCol = grayedOut ? sf::Color(120, 120, 130) : sf::Color::White;
                drawText(label, rect.position.x + 12.f, rect.position.y + rect.size.y / 2.f - 8.f, 13, lblCol, false, 0.f, 0.f);

                // Draw Value
                sf::Color valCol = grayedOut ? sf::Color(100, 100, 110) : (active ? sf::Color::Green : sf::Color::Red);
                if (grayedOut) {
                    // stays gray
                } else if (valStr != "ON" && valStr != "OFF") {
                    valCol = pal.uiAccent; // Cycled values get cyan/accent color
                }
                sf::Text tVal(font, valStr, 13);
                tVal.setFillColor(valCol);
                sf::FloatRect bVal = tVal.getLocalBounds();
                tVal.setOrigin({bVal.position.x + bVal.size.x, bVal.position.y + bVal.size.y / 2.f});
                tVal.setPosition({rect.position.x + rect.size.x - 12.f, rect.position.y + rect.size.y / 2.f});
                renderWindow.draw(tVal);
            };

            // Column 1
            drawToggleBtn(m_customUI.btnGravity, "GRAVITY", cfg.gravityEnabled ? "ON" : "OFF", cfg.gravityEnabled, hoveredCustomBtn == 0);
            drawToggleBtn(m_customUI.btnTactical, "TACTICAL STEP", cfg.tacticalStepEnabled ? "ON" : "OFF", cfg.tacticalStepEnabled, hoveredCustomBtn == 1);
            drawToggleBtn(m_customUI.btnSlot, "SLOT MACHINE", cfg.slotMachineEnabled ? "ON" : "OFF", cfg.slotMachineEnabled, hoveredCustomBtn == 2);
            drawToggleBtn(m_customUI.btnAscension, "ASCENSION", cfg.ascensionEnabled ? "ON" : "OFF", cfg.ascensionEnabled, hoveredCustomBtn == 3);

            // Column 2
            drawToggleBtn(m_customUI.btnGarbage, "GARBAGE BOARD", cfg.garbageBoardEnabled ? "ON" : "OFF", cfg.garbageBoardEnabled, hoveredCustomBtn == 4);
            
            std::string ratioStr = std::to_string(static_cast<int>(cfg.garbageRatio * 100)) + "%";
            drawToggleBtn(m_customUI.btnGarbageRatio, "GARBAGE RATIO", cfg.garbageBoardEnabled ? ratioStr : "N/A", true, hoveredCustomBtn == 5, !cfg.garbageBoardEnabled);
            
            drawToggleBtn(m_customUI.btnSpecial, "SPECIAL TILES", cfg.specialBlocksEnabled ? "ON" : "OFF", cfg.specialBlocksEnabled, hoveredCustomBtn == 6);

            std::string scoreStr = cfg.targetScoreEnabled ? std::to_string(cfg.targetScore) : "OFF";
            drawToggleBtn(m_customUI.btnScore, "SCORE TARGET", scoreStr, cfg.targetScoreEnabled, hoveredCustomBtn == 7);

            // Time Limit / Elapsed Time (Row 3, spans columns)
            std::string timeStr = cfg.timeLimitEnabled ? "COUNTDOWN (" + std::to_string(static_cast<int>(cfg.timeLimit)) + "s)" : "ELAPSED TIME";
            drawToggleBtn(m_customUI.btnTime, "TIME MODE", timeStr, cfg.timeLimitEnabled, hoveredCustomBtn == 8);

            // Bottom Buttons: START & BACK
            auto drawActionBtn = [&](sf::FloatRect rect, const std::string& txt, bool hover, sf::Color baseCol, int size) {
                sf::RectangleShape rs(rect.size);
                rs.setPosition(rect.position);
                rs.setFillColor(hover ? baseCol : pal.panelBorder);
                rs.setOutlineColor(baseCol);
                rs.setOutlineThickness(hover ? 3.f : 1.5f);
                renderWindow.draw(rs);

                sf::Color txtCol = hover ? pal.background : sf::Color::White;
                std::string drawTxt = hover ? "> " + txt + " <" : txt;
                drawText(drawTxt, rect.position.x + rect.size.x / 2.f, rect.position.y + rect.size.y / 2.f, size, txtCol, true);
            };

            drawActionBtn(m_customUI.btnStart, "START GAME", hoveredCustomBtn == 9, sf::Color::Green, 20);
            drawActionBtn(m_customUI.btnBack, "BACK", hoveredCustomBtn == 10, pal.uiAccent, 18);

            // Draw particles
            for (const auto& p : m_particles) {
                sf::RectangleShape pShape(sf::Vector2f{p.size, p.size});
                pShape.setOrigin({p.size * 0.5f, p.size * 0.5f});
                pShape.setPosition(p.position);
                
                float lifeRatio = 1.f - (p.age / p.lifetime);
                sf::Color c = p.color;
                c.a = static_cast<std::uint8_t>(p.color.a * lifeRatio);
                pShape.setFillColor(c);
                
                renderWindow.draw(pShape);
            }

            return;
        }

        if (m_showModeSelect) {
            // Draw title
            float tTitle = std::max(0.f, std::min(1.f, (m_introTimer + 0.1f) / 0.5f));
            float progressTitle = easeOutBack(tTitle);
            float yOffsetTitle = (1.f - progressTitle) * -150.f;
            drawText("SELECT GAME MODE", 350.f, 90.f, 36, pal.uiAccent, true, yOffsetTitle + m_mouseTilt.y * 1.2f, m_mouseTilt.x * 1.2f);

            // Draw cards
            std::vector<std::string> modeNames = {
                "CLASSIC",
                "GARBAGE CLEAR",
                "GARBAGE TIME",
                "SCORE TIME",
                "ASCENSION",
                "TACTICAL STEP",
                "TACTICAL GARBAGE",
                "CUSTOM"
            };

            std::vector<sf::Color> modeColors = {
                pal.pieceI,
                pal.pieceO,
                pal.pieceT,
                pal.pieceS,
                pal.pieceZ,
                pal.pieceJ,
                pal.pieceL,
                sf::Color::Magenta
            };

            for (int i = 0; i < 8; ++i) {
                int btnIdx = 10 + i;
                bool isHovered = (hoveredBtn == btnIdx);
                float localTime = m_introTimer - (i * 0.04f);
                float tVal = std::max(0.f, std::min(1.f, localTime / 0.4f));
                float animProgress = easeOutBack(tVal);

                float yOffset = (1.f - animProgress) * 350.f;
                float scale = btnScales[btnIdx] * animProgress;

                // Click feedback
                float clickScale = 1.f;
                float clickWiggle = 0.f;
                if (btnIdx == m_clickedBtn) {
                    clickScale = 0.85f + 0.15f * std::min(1.f, m_clickTimer / 0.15f);
                    clickWiggle = std::sin(m_clickTimer * 50.f) * 6.f * (1.f - std::min(1.f, m_clickTimer / 0.15f));
                }

                sf::Vector2f btnCenter = m_modeBtns[i].position + m_modeBtns[i].size * 0.5f + sf::Vector2f{0.f, yOffset} + m_mouseTilt * 0.15f;
                float swayAngle = std::sin(m_elapsedTime * 2.5f + i * 0.5f) * 1.5f + clickWiggle;

                sf::Transform transform;
                transform.translate(btnCenter);
                transform.rotate(sf::degrees(swayAngle));
                transform.scale({scale * clickScale, scale * clickScale});

                sf::RectangleShape rs(m_modeBtns[i].size);
                rs.setOrigin(m_modeBtns[i].size * 0.5f);
                rs.setPosition({0.f, 0.f});
                rs.setFillColor(isHovered ? pal.panelBg : pal.background);
                rs.setOutlineThickness(isHovered ? 2.5f : 1.5f);
                rs.setOutlineColor(isHovered ? pal.uiAccent : pal.panelBorder);
                renderWindow.draw(rs, transform);

                // Glow effect if hovered
                if (isHovered) {
                    for (int j = 0; j < 2; ++j) {
                        sf::RectangleShape glow(m_modeBtns[i].size + sf::Vector2f{j * 3.f, j * 3.f});
                        glow.setOrigin(glow.getSize() * 0.5f);
                        glow.setPosition({0.f, 0.f});
                        glow.setFillColor(sf::Color::Transparent);
                        sf::Color gc = pal.uiAccent;
                        gc.a = static_cast<std::uint8_t>(80 - j * 30);
                        glow.setOutlineColor(gc);
                        glow.setOutlineThickness(1.5f);
                        renderWindow.draw(glow, transform);
                    }
                }

                // Draw a small decorative color strip inside the card
                sf::RectangleShape strip({m_modeBtns[i].size.x - 20.f, 5.f});
                strip.setOrigin(strip.getSize() * 0.5f);
                strip.setPosition({0.f, -30.f});
                strip.setFillColor(modeColors[i]);
                renderWindow.draw(strip, transform);

                // Draw Card Title
                sf::Text t(font, modeNames[i], 15);
                t.setFillColor(isHovered ? pal.uiAccent : sf::Color::White);
                t.setStyle(sf::Text::Bold);
                sf::FloatRect b = t.getLocalBounds();
                t.setOrigin({b.position.x + b.size.x / 2.f, b.position.y + b.size.y / 2.f});
                t.setPosition({0.f, 10.f});
                renderWindow.draw(t, transform);
            }

            // Draw Description at the bottom
            std::string desc = "Hover over a mode card to view its description.";
            sf::Color descColor = sf::Color(160, 160, 180);
            if (hoveredBtn >= 10 && hoveredBtn <= 17) {
                descColor = pal.uiAccent;
                int idx = hoveredBtn - 10;
                std::vector<std::string> modeDescs = {
                    "Classic Tetris. Spin the slot machine with tokens to earn special powers!",
                    "40% of the board is filled with garbage. Clear the entire board to win!",
                    "40% garbage board. Clear all blocks before the countdown timer runs out!",
                    "Race against the clock to score 2000 points before the timer hits 0!",
                    "Endless Mode. No slot machine; choose 1 of 3 powers at milestones.",
                    "No gravity! The falling piece only drops when you place selection tiles.",
                    "40% garbage board & No gravity. Place selection pieces to drop and clear.",
                    "Custom Mode. Configure your own rules, toggles, limits, and options!"
                };
                desc = modeDescs[idx];
            }

            // Description background panel
            sf::RectangleShape descPanel({620.f, 50.f});
            descPanel.setOrigin(descPanel.getSize() * 0.5f);
            descPanel.setPosition({350.f, 570.f});
            descPanel.setFillColor(sf::Color(10, 10, 20, 200));
            descPanel.setOutlineThickness(1.f);
            descPanel.setOutlineColor(pal.panelBorder);
            renderWindow.draw(descPanel);

            drawText(desc, 350.f, 570.f, 13, descColor, true);

            // Draw Back Button
            int backBtnIdx = 18;
            bool isBackHovered = (hoveredBtn == backBtnIdx);
            float backTime = m_introTimer - (9 * 0.04f);
            float tBack = std::max(0.f, std::min(1.f, backTime / 0.4f));
            float backProgress = easeOutBack(tBack);

            float yOffsetBack = (1.f - backProgress) * 350.f;
            float backScale = btnScales[backBtnIdx] * backProgress;

            float clickScale = 1.f;
            float clickWiggle = 0.f;
            if (m_clickedBtn == backBtnIdx) {
                clickScale = 0.85f + 0.15f * std::min(1.f, m_clickTimer / 0.15f);
                clickWiggle = std::sin(m_clickTimer * 50.f) * 6.f * (1.f - std::min(1.f, m_clickTimer / 0.15f));
            }

            sf::Vector2f backCenter = m_btnBackModeSelect.position + m_btnBackModeSelect.size * 0.5f + sf::Vector2f{0.f, yOffsetBack} + m_mouseTilt * 0.15f;
            float backSway = std::sin(m_elapsedTime * 2.5f + 9 * 0.5f) * 1.5f + clickWiggle;

            sf::Transform backTransform;
            backTransform.translate(backCenter);
            backTransform.rotate(sf::degrees(backSway));
            backTransform.scale({backScale * clickScale, backScale * clickScale});

            sf::RectangleShape rsBack(m_btnBackModeSelect.size);
            rsBack.setOrigin(m_btnBackModeSelect.size * 0.5f);
            rsBack.setPosition({0.f, 0.f});
            rsBack.setFillColor(isBackHovered ? pal.uiAccent : pal.panelBorder);
            rsBack.setOutlineThickness(2.f);
            rsBack.setOutlineColor(pal.uiAccent);
            renderWindow.draw(rsBack, backTransform);

            sf::Color txtCol = isBackHovered ? pal.background : sf::Color::White;
            std::string btnText = isBackHovered ? ">  BACK  <" : "BACK";
            sf::Text tBackTxt(font, btnText, 20);
            tBackTxt.setFillColor(txtCol);
            sf::FloatRect bBack = tBackTxt.getLocalBounds();
            tBackTxt.setOrigin({bBack.position.x + bBack.size.x / 2.f, bBack.position.y + bBack.size.y / 2.f});
            tBackTxt.setPosition({0.f, 0.f});
            renderWindow.draw(tBackTxt, backTransform);

            // Draw particles on top of mode selection
            for (const auto& p : m_particles) {
                sf::RectangleShape pShape(sf::Vector2f{p.size, p.size});
                pShape.setOrigin({p.size * 0.5f, p.size * 0.5f});
                pShape.setPosition(p.position);
                
                float lifeRatio = 1.f - (p.age / p.lifetime);
                sf::Color c = p.color;
                c.a = static_cast<std::uint8_t>(p.color.a * lifeRatio);
                pShape.setFillColor(c);
                
                renderWindow.draw(pShape);
            }
            return;
        }

        auto drawBtn = [&](sf::FloatRect rect, const std::string &text, int btnIdx) {
            float localTime = m_introTimer - (btnIdx * 0.05f);
            float tVal = std::max(0.f, std::min(1.f, localTime / 0.4f));
            float animProgress = easeOutBack(tVal);

            float yOffset = (1.f - animProgress) * 350.f;
            float scale = btnScales[btnIdx] * animProgress;

            // Apply click feedback
            float clickScale = 1.f;
            float clickWiggle = 0.f;
            if (btnIdx == m_clickedBtn) {
                clickScale = 0.85f + 0.15f * std::min(1.f, m_clickTimer / 0.15f);
                clickWiggle = std::sin(m_clickTimer * 50.f) * 6.f * (1.f - std::min(1.f, m_clickTimer / 0.15f));
            }

            bool isHovered = (hoveredBtn == btnIdx);

            // Option A: Mouse-reactive parallax offset
            sf::Vector2f btnCenter = rect.position + rect.size * 0.5f + sf::Vector2f{0.f, yOffset} + m_mouseTilt * 0.2f;
            float swayAngle = std::sin(m_elapsedTime * 2.5f + btnIdx * 0.5f) * 1.5f + clickWiggle;

            sf::Transform transform;
            transform.translate(btnCenter);
            transform.rotate(sf::degrees(swayAngle));
            transform.scale({scale * clickScale, scale * clickScale});

            sf::RectangleShape rs(rect.size);
            rs.setOrigin(rect.size * 0.5f);
            rs.setPosition({0.f, 0.f});
            rs.setFillColor(isHovered ? pal.uiAccent : pal.panelBorder);
            rs.setOutlineThickness(2.f);
            rs.setOutlineColor(pal.uiAccent);
            renderWindow.draw(rs, transform);

            sf::Color txtCol = isHovered ? pal.background : sf::Color::White;

            std::string btnText = text;
            if (isHovered && btnIdx <= 2) {
                btnText = ">  " + text + "  <";
                txtCol = pal.background;
            }

            sf::Text t(font, btnText, 20);
            t.setFillColor(txtCol);
            sf::FloatRect b = t.getLocalBounds();
            t.setOrigin({b.position.x + b.size.x / 2.f, b.position.y + b.size.y / 2.f});
            t.setPosition({0.f, 0.f});
            renderWindow.draw(t, transform);
        };

        // Title drop-down bounce
        float tTetris = std::max(0.f, std::min(1.f, (m_introTimer + 0.1f) / 0.5f));
        float progressTetris = easeOutBack(tTetris);
        float yOffsetTetris = (1.f - progressTetris) * -200.f;

        float tReflex = std::max(0.f, std::min(1.f, (m_introTimer + 0.05f) / 0.5f));
        float progressReflex = easeOutBack(tReflex);
        float yOffsetReflex = (1.f - progressReflex) * -200.f;

        drawText("TETRIS", 354.f, 54.f, 60, pal.panelBorder, true, yOffsetTetris + m_mouseTilt.y * 1.5f, m_mouseTilt.x * 1.5f);
        drawText("TETRIS", 350.f, 50.f, 60, pal.uiAccent, true, yOffsetTetris + m_mouseTilt.y * 1.5f, m_mouseTilt.x * 1.5f);
        drawText("REFLEX", 350.f, 100.f, 40, pal.pieceI, true, yOffsetReflex + m_mouseTilt.y * 0.8f, m_mouseTilt.x * 0.8f);

        // COLOR PALETTE Header Text
        float tPalText = std::max(0.f, std::min(1.f, (m_introTimer - 0.2f) / 0.4f));
        float progressPalText = easeOutBack(tPalText);
        float yOffsetPalText = (1.f - progressPalText) * 150.f;
        sf::Color palTextCol = sf::Color::White;
        palTextCol.a = static_cast<std::uint8_t>(255 * tPalText);
        drawText("COLOR PALETTE", 350.f, 150.f, 16, palTextCol, true, yOffsetPalText + m_mouseTilt.y * 0.5f, m_mouseTilt.x * 0.5f);

        // YENİ: Detaylı Palet Çizimi (Balatro-style transforms)
        for (int i = 0; i < 4; ++i) {
            int btnIdx = 5 + i;
            const auto &tp = PALETTES[i];
            bool isActive = (settings->activePalette == i);
            bool isHovered = (hoveredBtn == btnIdx);

            float localTime = m_introTimer - (btnIdx * 0.05f);
            float tVal = std::max(0.f, std::min(1.f, localTime / 0.4f));
            float animProgress = easeOutBack(tVal);

            float yOffset = (1.f - animProgress) * 350.f;
            float scale = btnScales[btnIdx] * animProgress;

            // Apply click feedback
            float clickScale = 1.f;
            float clickWiggle = 0.f;
            if (btnIdx == m_clickedBtn) {
                clickScale = 0.85f + 0.15f * std::min(1.f, m_clickTimer / 0.15f);
                clickWiggle = std::sin(m_clickTimer * 50.f) * 6.f * (1.f - std::min(1.f, m_clickTimer / 0.15f));
            }

            sf::Vector2f btnCenter = paletteBtns[i].position + paletteBtns[i].size * 0.5f + sf::Vector2f{0.f, yOffset} + m_mouseTilt * 0.2f;
            float swayAngle = std::sin(m_elapsedTime * 2.5f + btnIdx * 0.5f) * 1.5f + clickWiggle;

            sf::Transform transform;
            transform.translate(btnCenter);
            transform.rotate(sf::degrees(swayAngle));
            transform.scale({scale * clickScale, scale * clickScale});

            sf::RectangleShape rs(paletteBtns[i].size);
            rs.setOrigin(paletteBtns[i].size * 0.5f);
            rs.setPosition({0.f, 0.f});
            rs.setFillColor(isHovered
                                ? sf::Color(static_cast<std::uint8_t>(std::min(255, tp.panelBg.r + 30)),
                                            static_cast<std::uint8_t>(std::min(255, tp.panelBg.g + 30)),
                                            static_cast<std::uint8_t>(std::min(255, tp.panelBg.b + 30)))
                                : tp.panelBg);
            rs.setOutlineThickness(isActive ? 2.f : 1.f);
            rs.setOutlineColor(isActive ? pal.uiAccent : pal.panelBorder);
            renderWindow.draw(rs, transform);

            auto drawScaledStrip = [&](float localX, sf::Color color) {
                sf::RectangleShape strip(sf::Vector2f{10.f, paletteBtns[i].size.y - 4.f});
                strip.setOrigin({5.f, (paletteBtns[i].size.y - 4.f) * 0.5f});
                strip.setPosition({localX, 0.f});
                strip.setFillColor(color);
                renderWindow.draw(strip, transform);
            };

            drawScaledStrip(-40.f, tp.background);
            drawScaledStrip(-28.f, tp.pieceI);
            drawScaledStrip(-16.f, tp.pieceT);

            // Palet İsmi
            sf::Text t(font, tp.name.substr(0, 8) + ".", 12);
            t.setFillColor(tp.uiText);
            sf::FloatRect b = t.getLocalBounds();
            t.setOrigin({b.position.x + b.size.x / 2.f, b.position.y + b.size.y / 2.f});
            t.setPosition({25.f, 0.f});
            renderWindow.draw(t, transform);
        }

        std::string sfxText = settings->sfxEnabled ? "SFX: ON" : "SFX: OFF";
        std::string musText = settings->musicEnabled ? "MUS: ON" : "MUS: OFF";
        drawBtn(btnSfx, sfxText, 3);
        drawBtn(btnMusic, musText, 4);

        drawBtn(btnNewGame, "NEW GAME", 0);
        drawBtn(btnHighScores, "HIGH SCORES", 1);
        drawBtn(btnQuit, "QUIT", 2);

        // F11 Fade In
        float tHelpText = std::max(0.f, std::min(1.f, (m_introTimer - 0.5f) / 0.4f));
        sf::Color helpCol = pal.uiAccent;
        helpCol.a = static_cast<std::uint8_t>(255 * tHelpText);
        drawText("F11 - Toggle Fullscreen", 350.f, 680.f, 14, helpCol, true);

        // Option B: Render Hover Spark Trails
        for (const auto& p : m_particles) {
            sf::RectangleShape pShape(sf::Vector2f{p.size, p.size});
            pShape.setOrigin({p.size * 0.5f, p.size * 0.5f});
            pShape.setPosition(p.position);
            
            float lifeRatio = 1.f - (p.age / p.lifetime);
            sf::Color c = p.color;
            c.a = static_cast<std::uint8_t>(p.color.a * lifeRatio);
            pShape.setFillColor(c);
            
            renderWindow.draw(pShape);
        }

        if (showScoreTable) {
            float t = std::max(0.f, std::min(1.f, m_scoreTableTimer / 0.4f));
            float animProgress = easeOutBack(t);
            float yOffset = (1.f - animProgress) * -740.f; // slide down from top

            sf::Transform transform;
            transform.translate({0.f, yOffset});
            renderScoreTable(renderWindow, transform);
        }
    }

    void MainMenuScene::renderScoreTable(sf::RenderWindow &renderWindow, const sf::Transform& transform) {
        sf::RectangleShape dim({700.f, 740.f});
        dim.setFillColor(sf::Color(0, 0, 0, 245));
        renderWindow.draw(dim, transform);

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
            renderWindow.draw(t, transform);
        };

        drawText("STATISTICS", 350.f, 50.f, 36, PALETTES[settings->activePalette].uiAccent, true);

        // Render cycle mode name
        std::string modeText = getModeName(m_scoreTableMode);
        drawText(modeText, 350.f, 105.f, 18, sf::Color::Cyan, true);

        // Cycle arrows
        auto drawArrowBtn = [&](sf::FloatRect rect, const std::string &txt, bool hover) {
            sf::RectangleShape rs(rect.size);
            rs.setPosition(rect.position);
            rs.setFillColor(hover ? PALETTES[settings->activePalette].uiAccent : sf::Color(30, 30, 40));
            rs.setOutlineColor(PALETTES[settings->activePalette].uiAccent);
            rs.setOutlineThickness(1.f);
            renderWindow.draw(rs, transform);
            
            sf::Color txtCol = hover ? PALETTES[settings->activePalette].background : sf::Color::White;
            drawText(txt, rect.position.x + rect.size.x / 2.f, rect.position.y + rect.size.y / 2.f - 2.f, 16, txtCol, true);
        };
        
        drawArrowBtn(btnScorePrev, "<", prevScoreHovered);
        drawArrowBtn(btnScoreNext, ">", nextScoreHovered);

        const auto& modeStats = m_gameStats[m_scoreTableMode];

        float startY = 150.f;
        drawText("GAMES PLAYED: " + std::to_string(modeStats.playCount), 350.f, startY, 20, sf::Color::White, true);

        bool isScoreBased = (m_scoreTableMode == GameMode::Classic ||
                             m_scoreTableMode == GameMode::ScoreTimeAttack ||
                             m_scoreTableMode == GameMode::Ascension ||
                             m_scoreTableMode == GameMode::TacticalStep);

        if (modeStats.playCount > 0 && !modeStats.history.empty()) {
            int bestVal = modeStats.history[0];
            int worstVal = modeStats.history[0];
            for (int val : modeStats.history) {
                if (isScoreBased) {
                    if (val > bestVal) bestVal = val;
                    if (val < worstVal) worstVal = val;
                } else {
                    if (val < bestVal) bestVal = val; // lower is better
                    if (val > worstVal) worstVal = val; // higher is worse
                }
            }

            std::string bestStr = "BEST PERFORMANCE: " + StatsManager::formatValue(m_scoreTableMode, bestVal);
            std::string worstStr = "WORST PERFORMANCE: " + StatsManager::formatValue(m_scoreTableMode, worstVal);
            drawText(bestStr, 350.f, startY + 30.f, 18, sf::Color(100, 255, 100), true);
            drawText(worstStr, 350.f, startY + 55.f, 18, sf::Color(255, 100, 100), true);

            // ── DRAW GRAPH ──
            float gx = 80.f, gy = 260.f;
            float gw = 540.f, gh = 240.f;

            // Draw graph background box
            sf::RectangleShape graphBg({gw, gh});
            graphBg.setPosition({gx, gy});
            graphBg.setFillColor(sf::Color(15, 15, 25, 200));
            graphBg.setOutlineColor(PALETTES[settings->activePalette].panelBorder);
            graphBg.setOutlineThickness(1.5f);
            renderWindow.draw(graphBg, transform);

            // Draw grid lines inside graph
            sf::RectangleShape gridLine;
            gridLine.setFillColor(sf::Color(50, 50, 70, 100));
            // 4 vertical lines
            for (int i = 1; i <= 4; ++i) {
                gridLine.setSize({1.f, gh});
                gridLine.setPosition({gx + i * (gw / 5.f), gy});
                renderWindow.draw(gridLine, transform);
            }
            // 4 horizontal lines
            for (int i = 1; i <= 4; ++i) {
                gridLine.setSize({gw, 1.f});
                gridLine.setPosition({gx, gy + i * (gh / 5.f)});
                renderWindow.draw(gridLine, transform);
            }

            // Find min/max values in history for scaling
            int minHVal = modeStats.history[0];
            int maxHVal = modeStats.history[0];
            for (int val : modeStats.history) {
                if (val < minHVal) minHVal = val;
                if (val > maxHVal) maxHVal = val;
            }

            int valRange = maxHVal - minHVal;
            int nPoints = static_cast<int>(modeStats.history.size());

            // Draw line strip
            std::vector<sf::Vector2f> points;
            for (int i = 0; i < nPoints; ++i) {
                float px = gx;
                if (nPoints > 1) {
                    px = gx + i * (gw / (nPoints - 1));
                } else {
                    px = gx + gw / 2.f;
                }
                
                float py = gy + gh / 2.f;
                if (valRange > 0) {
                    py = (gy + gh) - (static_cast<float>(modeStats.history[i] - minHVal) / valRange) * gh;
                }
                points.push_back({px, py});
            }

            // Draw lines connecting points with glowing circles
            sf::Color lineColor = PALETTES[settings->activePalette].uiAccent;
            for (size_t i = 0; i < points.size(); ++i) {
                if (i > 0) {
                    sf::Vector2f p1 = points[i - 1];
                    sf::Vector2f p2 = points[i];

                    sf::Vertex lineVerts[2];
                    lineVerts[0].position = p1;
                    lineVerts[0].color = lineColor;
                    lineVerts[1].position = p2;
                    lineVerts[1].color = lineColor;

                    renderWindow.draw(lineVerts, 2, sf::PrimitiveType::Lines, transform);
                }

                float dotRadius = 4.f;
                sf::CircleShape dot(dotRadius);
                dot.setOrigin({dotRadius, dotRadius});
                dot.setPosition(points[i]);
                dot.setFillColor(sf::Color::White);
                dot.setOutlineColor(lineColor);
                dot.setOutlineThickness(1.5f);
                renderWindow.draw(dot, transform);
            }
            
            drawText("CHRONOLOGICAL PERFORMANCE TREND", 350.f, gy + gh + 15.f, 12, sf::Color(140, 140, 160), true);

        } else {
            drawText("NO HISTORY YET", 350.f, 350.f, 22, sf::Color(150, 150, 150), true);
            drawText("Play a game in this mode to see stats and trend graph!", 350.f, 390.f, 13, sf::Color(120, 120, 120), true);
        }

        // Back button
        sf::RectangleShape rs(btnBack.size);
        rs.setPosition(btnBack.position);
        rs.setFillColor(backHovered
                            ? PALETTES[settings->activePalette].uiAccent
                            : PALETTES[settings->activePalette].panelBorder);
        rs.setOutlineThickness(2.f);
        rs.setOutlineColor(PALETTES[settings->activePalette].uiAccent);
        renderWindow.draw(rs, transform);

        sf::Color txtCol = backHovered ? PALETTES[settings->activePalette].background : sf::Color::White;
        drawText("BACK", btnBack.position.x + btnBack.size.x / 2.f, btnBack.position.y + btnBack.size.y / 2.f, 20,
                 txtCol, true);
    }
} // namespace tetris//
// Created by e-r-e on 5/20/2026.
//
