#pragma once

#include "../SceneManager.hpp"
#include "MainMenuScene.hpp"
#include "GameState.hpp"
#include "BoardRenderer.hpp"
#include "SelectionBar.hpp"
#include "../ParticleSystem.hpp"
#include "../AudioManager.hpp"
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <ctime>
#include <memory>
#include <string>
#include "ColorPalette.hpp"
#include "PauseMenu.hpp"
#include "TetrisHUD.hpp"
#include "TetrisConstants.hpp"
#include "tetris/AppSettings.hpp"
#include "tetris/ScoreManager.hpp"
#include "../WindowUtils.hpp"
#include <functional>
#include <cctype>

namespace tetris {
    struct FloatingTextComponent {
        std::string text;
        sf::Color color;
        float lifetime{1.2f};
        float maxLifetime{1.2f};
        sf::Vector2f velocity{0.f, -60.f};
        unsigned int fontSize{18};
    };

    class TetrisScene : public engine::Scene {
    public:
        explicit TetrisScene(sf::RenderWindow &win, AppSettings *appSettings)
            : window(win)
              , settings(appSettings)
              , gameState(static_cast<std::uint32_t>(std::time(nullptr)))
              , boardRenderer({BOARD_X, BOARD_Y})
              , selectionBar({0.f, BAR_Y}, float(WIN_W)) {
            playerName = settings->playerName;
            setupCallbacks();
        }

        void onEnter() override {
            // Register ECS components
            registry().registerComponent<engine::TransformComponent>();
            registry().registerComponent<engine::EmitterComponent>();
            registry().registerComponent<tetris::FloatingTextComponent>();

            // Instantiate ECS systems
            particleSystem = std::make_unique<engine::ParticleSystem>(registry());

            bool loaded = false;
            for (auto &path: {
                     std::string("assets/font.ttf"),
                     std::string("C:/Windows/Fonts/arial.ttf"),
                     std::string("C:/Windows/Fonts/calibri.ttf"),
                     std::string("C:/Windows/Fonts/segoeui.ttf")
                 }) {
                if (loaded) break;
                if (font.openFromFile(path)) loaded = true;
            }
            if (loaded) {
                selectionBar.setFont(font);
                fontLoaded = true;
                hud.setFont(font);
                boardRenderer.setFont(font);
            }
            powerSelectMode = false;
            selectedPowerIdx = -1;
            showVictory = false;
            showAscensionModal = false;
            m_statsSaved = false;
            m_newBest = false;
            m_newWorst = false;

            gameState.gameMode = settings->activeMode;
            if (gameState.gameMode == GameMode::GarbageClear) {
                gameState.setupGarbageBoard(settings->garbageRatio);
                gameState.countdownTimer = 0.f;
                gameState.fallInterval = 2.0f;
            } else if (gameState.gameMode == GameMode::GarbageTimeAttack) {
                gameState.setupGarbageBoard(settings->garbageRatio);
                gameState.countdownTimer = 240.f;
            } else if (gameState.gameMode == GameMode::ScoreTimeAttack) {
                gameState.countdownTimer = settings->scoreTimeLimit;
                gameState.targetScore = settings->scoreTimeTarget;
            } else if (gameState.gameMode == GameMode::Ascension) {
                gameState.countdownTimer = 90.f;
                gameState.targetScore = 1500;
            } else if (gameState.gameMode == GameMode::TacticalStep) {
                gameState.countdownTimer = 0.f;
            } else if (gameState.gameMode == GameMode::TacticalGarbageClear) {
                gameState.setupGarbageBoard(settings->garbageRatio);
                gameState.countdownTimer = 0.f;
            } else if (gameState.gameMode == GameMode::Custom) {
                gameState.customSettings = settings->customSettings;
                if (gameState.customSettings.garbageBoardEnabled) {
                    gameState.setupGarbageBoard(gameState.customSettings.garbageRatio);
                } else {
                    gameState.board.clear();
                }
                if (gameState.customSettings.timeLimitEnabled) {
                    gameState.countdownTimer = gameState.customSettings.timeLimit;
                } else {
                    gameState.countdownTimer = 0.f;
                }
                if (gameState.customSettings.targetScoreEnabled) {
                    gameState.targetScore = gameState.customSettings.targetScore;
                } else {
                    gameState.targetScore = 0;
                }
                if (gameState.customSettings.gravityEnabled) {
                    gameState.fallInterval = 1.0f;
                } else {
                    gameState.fallInterval = 999999.f;
                }
            } else { // Classic
                gameState.countdownTimer = 0.f;
            }

            gameState.onGameWon = [this]() {
                showVictory = true;
                playerName = "";
                audio.playSound("levelup");
                audio.stopMusic(2.f);
                saveStats();
            };

            // ── Ses efektlerini yükle ──────────────────────────────────────
            audio.loadSound("land", "assets/sounds/piece_land.wav");
            audio.loadSound("clear1", "assets/sounds/line_clear.wav");
            audio.loadSound("clear4", "assets/sounds/line_clear_4.wav");
            audio.loadSound("levelup", "assets/sounds/level_up.wav");
            audio.loadSound("token", "assets/sounds/token_earn.wav");
            audio.loadSound("spin", "assets/sounds/slot_spin.wav");
            audio.loadSound("gameover", "assets/sounds/game_over.wav");
            audio.setVolume(engine::AudioBus::Music, 45.f);
            audio.setVolume(engine::AudioBus::SFX, 85.f);
            audio.playMusic("assets/music/bgm.ogg", true, 0.f);

            // Ayarları uygula
            applyPalette(settings->activePalette);
            sfxEnabled = settings->sfxEnabled;
            musicEnabled = settings->musicEnabled;
            audio.setVolume(engine::AudioBus::SFX, sfxEnabled ? 85.f : 0.f);
            audio.setVolume(engine::AudioBus::Music, musicEnabled ? 45.f : 0.f);
        }

        void update(float dt, engine::InputManager &input) override {
            // Update Pause Menu Timers
            pauseMenu.update(dt, gameState.paused);

            // Mouse-Reactive Board Tilt Parallax
            sf::Vector2f mouseCoords = window.mapPixelToCoords(sf::Mouse::getPosition(window));
            sf::Vector2f screenCenter(WIN_W * 0.5f, WIN_H * 0.5f);
            sf::Vector2f mouseDiff = mouseCoords - screenCenter;

            float targetTiltX = mouseDiff.x * 0.03f;
            float targetTiltY = mouseDiff.y * 0.03f;
            float targetRotation = mouseDiff.x * 0.005f;

            m_mouseTilt.x += (targetTiltX - m_mouseTilt.x) * 8.f * dt;
            m_mouseTilt.y += (targetTiltY - m_mouseTilt.y) * 8.f * dt;
            m_tiltRotation += (targetRotation - m_tiltRotation) * 8.f * dt;

            if (gameState.gameOver || gameState.gameWon) {
                audio.update(dt);
                return;
            }

            if (showAscensionModal) {
                audio.update(dt);
                return;
            }

            if (showPowerModal) {
                audio.update(dt);
                selectionBar.update(dt, gameState);
                return;
            }

            if (isSpinning) {
                spinWaitTimer -= dt;
                if (isFree) {
                    if (spinWaitTimer <= 0.f) {
                        isSpinning = false;
                        slotResultTimer = 2.5f;
                        gameState.freezeFalling(false);
                    }
                }
                else {
                    if (spinWaitTimer <= 0.f) {
                        isSpinning = false; // Animasyon bitti!

                        if (lastSlotResult.hasCombo) {
                            showPowerModal = true; // Animasyon bittiği an modalı patlat
                        } else {
                            slotResultTimer = 2.5f; // Kombo yoksa banner çıksın
                            gameState.freezeFalling(false); // Ve oyun düşmeye devam etsin
                        }
                    }
                }

            }

            if (input.isPressed(sf::Keyboard::Key::P)) {
                if (gameState.paused) {
                    pauseMenu.triggerOutro([this]() {
                        gameState.paused = false;
                    });
                } else {
                    gameState.paused = true;
                }
            }

            int levelBefore = gameState.level;
            gameState.update(dt);
            selectionBar.update(dt, gameState);

            // Win Condition check
            bool isTacticalGarbage = (gameState.gameMode == GameMode::TacticalGarbageClear);
            bool hasGarbageMode = (gameState.gameMode == GameMode::GarbageClear || 
                                   gameState.gameMode == GameMode::GarbageTimeAttack);
            if (isTacticalGarbage) {
                if (gameState.board.countOriginalGarbage() == 0 && !gameState.gameWon) {
                    gameState.gameWon = true;
                    if (gameState.onGameWon) gameState.onGameWon();
                }
            } else if (hasGarbageMode && gameState.isBoardEmpty() && !gameState.gameWon) {
                gameState.gameWon = true;
                if (gameState.onGameWon) gameState.onGameWon();
            }

            if (gameState.gameMode == GameMode::ScoreTimeAttack && 
                gameState.score >= gameState.targetScore && !gameState.gameWon) {
                gameState.gameWon = true;
                if (gameState.onGameWon) gameState.onGameWon();
            }

            // Custom Mode Win Conditions
            if (gameState.gameMode == GameMode::Custom) {
                const auto& cfg = gameState.customSettings;
                bool shouldWin = false;
                
                if (cfg.garbageBoardEnabled) {
                    if (cfg.tacticalStepEnabled) {
                        if (gameState.board.countOriginalGarbage() == 0) {
                            shouldWin = true;
                        }
                    } else {
                        if (gameState.isBoardEmpty()) {
                            shouldWin = true;
                        }
                    }
                }
                
                if (cfg.targetScoreEnabled && gameState.score >= gameState.targetScore) {
                    if (cfg.ascensionEnabled) {
                        if (!showAscensionModal) {
                            openAscensionModal();
                        }
                    } else {
                        shouldWin = true;
                    }
                }
                
                if (shouldWin && !gameState.gameWon) {
                    gameState.gameWon = true;
                    if (gameState.onGameWon) gameState.onGameWon();
                }
            }

            // Ascension Milestone check
            if (gameState.gameMode == GameMode::Ascension && 
                gameState.score >= gameState.targetScore && !showAscensionModal) {
                openAscensionModal();
            }

            if (gameState.level > levelBefore) {
                audio.playSound("levelup");
                spawnFloatingText({BOARD_X + BOARD_PIXEL_W * 0.5f, BOARD_Y + BOARD_PIXEL_H * 0.35f}, "LEVEL UP!", sf::Color(100, 255, 100));
            }

            audio.update(dt);
            hud.update(dt);
            boardRenderer.update(dt);

            if (trauma > 0.f) {
                trauma = std::max(0.f, trauma - dt * 2.0f);
            }

            if (isSpinning) {
                addSpinSparks();
            }

            updateFlash(dt);
            updateDragPreview();

            if (particleSystem) {
                particleSystem->update(dt);
            }

            // Update ECS floating texts
            auto floatSig = registry().buildSignature<engine::TransformComponent, tetris::FloatingTextComponent>();
            std::vector<engine::Entity> toDestroy;
            for (engine::Entity e : registry().view(floatSig)) {
                auto& tf = registry().getComponent<engine::TransformComponent>(e);
                auto& ft = registry().getComponent<tetris::FloatingTextComponent>(e);

                ft.lifetime -= dt;
                if (ft.lifetime <= 0.f) {
                    toDestroy.push_back(e);
                } else {
                    tf.position += ft.velocity * dt;
                }
            }
            for (engine::Entity e : toDestroy) {
                registry().destroyEntity(e);
            }

            arrowPhase += dt / 1.2f;
            if (arrowPhase > 1.f) arrowPhase -= 1.f;

            if (tokenMilestoneNotifTimer > 0.f) tokenMilestoneNotifTimer -= dt;
            if (slotResultTimer > 0.f) slotResultTimer -= dt;
        }

        void handleEvent(const sf::Event &event) {
            if (gameState.gameOver) {
                handleGameOverEvent(event);
                return;
            }

            if (showVictory) {
                handleGameOverEvent(event);
                return;
            }

            if (showAscensionModal) {
                if (const auto *e = event.getIf<sf::Event::MouseButtonPressed>()) {
                    if (e->button == sf::Mouse::Button::Left) {
                        sf::Vector2f mouse = window.mapPixelToCoords(sf::Mouse::getPosition(window));
                        for (int i = 0; i < 3; ++i) {
                            if (ascensionCardRects[i].contains(mouse)) {
                                chooseAscensionPower(i);
                                break;
                            }
                        }
                    }
                }
                return;
            }

            if (gameState.paused) {
                sf::Vector2f mouse = window.mapPixelToCoords(sf::Mouse::getPosition(window));
                if (event.is<sf::Event::MouseMoved>()) {
                    pauseMenu.handleMouseMove(mouse);
                } else if (const auto *e = event.getIf<sf::Event::MouseButtonPressed>()) {
                    if (e->button == sf::Mouse::Button::Left) {
                        pauseMenu.handleMouseClick(mouse);
                    }
                }
                return;
            }

            if (const auto *e = event.getIf<sf::Event::KeyPressed>()) {
                if (e->code == sf::Keyboard::Key::F1) {
                    showDebugMenu = !showDebugMenu;
                    return;
                }
                
                if (!gameState.paused && !gameState.gameOver && !showPowerModal && !showDebugMenu) {
                    if (e->code == sf::Keyboard::Key::Space && gameState.isHardFallAllowed() && !showAscensionModal && !isSpinning && !gameState.gameWon) {
                        gameState.hardDropFallingPiece();
                    }
                    else if (e->code == sf::Keyboard::Key::Num1) { usePowerHotkey(0); }
                    else if (e->code == sf::Keyboard::Key::Num2) { usePowerHotkey(1); }
                    else if (e->code == sf::Keyboard::Key::Num3) { usePowerHotkey(2); }
                    else if (e->code == sf::Keyboard::Key::Num4) { usePowerHotkey(3); }
                    else if (e->code == sf::Keyboard::Key::Num5) { usePowerHotkey(4); }
                }
            }

            sf::Vector2f mouse = window.mapPixelToCoords(sf::Mouse::getPosition(window));

            // YENİ: Debug menüsü açıksa tıklamaları sadece o menü için işle
            if (showDebugMenu) {
                if (const auto *e = event.getIf<sf::Event::MouseButtonPressed>()) {
                    if (e->button == sf::Mouse::Button::Left) {
                        float startX = 60.f, startY = 80.f;
                        float btnW = 180.f, btnH = 40.f;
                        float spacingX = 15.f, spacingY = 15.f;

                        // 15 adet gücün konumunu kontrol et (3 sütun, 5 satır)
                        for (int i = 0; i < 15; ++i) {
                            int col = i % 3;
                            int row = i / 3;
                            sf::FloatRect btnRect({startX + col * (btnW + spacingX), startY + row * (btnH + spacingY)},
                                                  {btnW, btnH});

                            if (btnRect.contains(mouse)) {
                                PowerType p = static_cast<PowerType>(i);
                                PowerSlot dummySlot;
                                dummySlot.type = p;

                                // Efekti oynat ve gücü direkt çalıştır!
                                addPowerBlastParticles(dummySlot.color());
                                applyPowerEffect(p);

                                // Menüyü açık bırakıyoruz ki arka arkaya test edebilesin
                                return;
                            }
                        }
                    }
                }
                return; // Debug menüsü açıkken oyun gridine veya barlara tıklanmasını engelle
            }

            // YENİ: Modal açıksa diğer eventleri yut ve kendi butonlarını kontrol et
            if (showPowerModal) {
                if (const auto *e = event.getIf<sf::Event::MouseButtonPressed>()) {
                    if (e->button == sf::Mouse::Button::Left) {
                        if (btnUseNowRect.contains(mouse)) {
                            // "ŞİMDİ KULLAN" seçildi
                            showPowerModal = false;
                            gameState.freezeFalling(false);

                            // YENİ: Listeye HİÇ eklemeden direkt havada kullan!
                            addPowerBlastParticles(pendingPower.color());
                            applyPowerEffect(pendingPower.type);
                        } else if (btnKeepRect.contains(mouse)) {
                            // "SAKLA" seçildi
                            showPowerModal = false;
                            gameState.freezeFalling(false);

                            // YENİ: Sadece sakla dediğinde listeye ekle!
                            gameState.slotMachine.awardPower(pendingPower.type);
                        }
                    }
                }
                return; // Modal açıkken arkaya tıklanmasını engelle
            }

            if (const auto *e = event.getIf<sf::Event::MouseButtonPressed>()) {
                if (e->button == sf::Mouse::Button::Left) {
                    if (oPlacementMode) {
                        sf::FloatRect br = boardRenderer.boardRect();
                        if (br.contains(mouse)) {
                            int col = boardRenderer.screenXToCol(mouse.x);
                            gameState.applyMoveFallingToCol(col);
                        }
                        gameState.freezeFalling(false);
                        oPlacementMode = false;
                        return;
                    }

                    if (mouse.x < LEFT_PANEL_W && mouse.y < BAR_Y) {
                        bool isSlotMachineEnabled = true;
                        if (gameState.gameMode == GameMode::Custom && !gameState.customSettings.slotMachineEnabled) {
                            isSlotMachineEnabled = false;
                        }
                        if (isSlotMachineEnabled) {
                            if (powerSelectMode) handlePowerClick(mouse);
                            else handleLeftPanelToggle(mouse);
                        }
                        return;
                    }

                    if (powerSelectMode) {
                        powerSelectMode = false;
                        gameState.freezeFalling(false);
                        return;
                    }

                    selectionBar.onMousePress(mouse, gameState);
                }
                if (e->button == sf::Mouse::Button::Right)
                    selectionBar.onRightClick(mouse, gameState);
            }

            if (event.is<sf::Event::MouseMoved>()) {
                selectionBar.onMouseMove(mouse);
                
                // Mouse hover check for left panel powers
                hoveredPowerIdx = -1;
                bool isSlotMachineEnabled = true;
                if (gameState.gameMode == GameMode::Custom && !gameState.customSettings.slotMachineEnabled) {
                    isSlotMachineEnabled = false;
                }
                if (mouse.x < LEFT_PANEL_W && isSlotMachineEnabled) {
                    auto &powers = gameState.slotMachine.powers;
                    for (int i = 0; i < static_cast<int>(powers.size()); ++i) {
                        float y = BOARD_Y + 28.f + i * 44.f;
                        if (mouse.x >= 4.f && mouse.x < LEFT_PANEL_W - 4.f &&
                            mouse.y >= y && mouse.y < y + 38.f) {
                            hoveredPowerIdx = i;
                            break;
                        }
                    }
                }
            }

            if (const auto *e = event.getIf<sf::Event::MouseButtonReleased>()) {
                if (e->button == sf::Mouse::Button::Left) {
                    auto result = selectionBar.onMouseRelease(
                        mouse, BOARD_X, BOARD_Y, BOARD_Y + BOARD_PIXEL_H, gameState);
                    if (result.success) {
                        addPlaceParticles(mouse);
                        auto dragPiece = selectionBar.dragState().piece;
                        dragPiece.col = result.boardCol;
                        dragPiece.row = result.boardRow;
                        auto cells = dragPiece.cells();
                        if (!cells.empty()) {
                            float sumX = 0.f, sumY = 0.f;
                            for (auto [r, c] : cells) {
                                sumX += (BOARD_X + c * CELL_SIZE + CELL_SIZE * 0.5f);
                                sumY += (BOARD_Y + r * CELL_SIZE + CELL_SIZE * 0.5f);
                            }
                            sf::Vector2f center(sumX / cells.size(), sumY / cells.size());
                            boardRenderer.triggerPlacementEffect(center, dragPiece.color());
                            boardRenderer.triggerPlacementScalePop(cells, gameState.board);
                        }
                    }
                    boardRenderer.setDragPreview(std::nullopt, true);
                }
            }
        }

        void render(sf::RenderWindow &renderWindow, float) override {
            sf::Color letterboxCol = PALETTES[activePalette].panelBorder;
            letterboxCol.r /= 2; letterboxCol.g /= 2; letterboxCol.b /= 2;
            renderWindow.clear(letterboxCol);

            // Establish the correct aspect-ratio corrected view
            updateViewport(renderWindow, float(WIN_W), float(WIN_H));
            sf::View letterboxView = renderWindow.getView();

            sf::View shakeView = letterboxView;
            if (trauma > 0.f) {
                float shakeOffset = trauma * trauma * 15.f;
                float shakeAngle = trauma * trauma * 4.f;
                float offsetX = ((rand() % 200 - 100) / 100.f) * shakeOffset;
                float offsetY = ((rand() % 200 - 100) / 100.f) * shakeOffset;
                float angle = ((rand() % 200 - 100) / 100.f) * shakeAngle;
                shakeView.move({offsetX + m_mouseTilt.x, offsetY + m_mouseTilt.y});
                shakeView.setRotation(sf::degrees(angle + m_tiltRotation));
            } else {
                shakeView.move(m_mouseTilt);
                shakeView.setRotation(sf::degrees(m_tiltRotation));
            }
            renderWindow.setView(shakeView);

            sf::RectangleShape mainBg({float(WIN_W), float(WIN_H)});
            mainBg.setFillColor(PALETTES[activePalette].background);
            renderWindow.draw(mainBg);

            boardRenderer.setFlashRows(flashRows,
                                       flashDuration > 0 ? flashTimer / flashDuration : 0.f);
            boardRenderer.render(renderWindow, gameState.board,
                                 gameState.fallingPiece, true);

            if (!gameState.gameOver && !gameState.paused)
                renderNextPieceArrow(renderWindow);

            if (gameState.fallingHasToken)
                drawTokenGlow(renderWindow);

            if (particleSystem) {
                particleSystem->render(renderWindow);
            }

            // Draw ECS floating texts
            auto floatSig = registry().buildSignature<engine::TransformComponent, tetris::FloatingTextComponent>();
            for (engine::Entity e : registry().view(floatSig)) {
                auto& tf = registry().getComponent<engine::TransformComponent>(e);
                auto& ft = registry().getComponent<tetris::FloatingTextComponent>(e);

                if (fontLoaded) {
                    sf::Text text(font, ft.text, ft.fontSize);
                    sf::Color c = ft.color;
                    float ratio = ft.lifetime / ft.maxLifetime;
                    c.a = static_cast<std::uint8_t>(ratio * 255);
                    text.setFillColor(c);
                    text.setStyle(sf::Text::Bold);
                    sf::FloatRect bounds = text.getLocalBounds();
                    text.setOrigin({bounds.size.x * 0.5f, bounds.size.y * 0.5f});
                    text.setPosition(tf.position);
                    renderWindow.draw(text);
                }
            }

            selectionBar.render(renderWindow, gameState);

            // Reset view for static UI panels and menus
            renderWindow.setView(letterboxView);

            // Time Freeze Overlay (Icy Board Vignette & Timer)
            if (gameState.timeStopTimer > 0.f) {
                sf::RectangleShape boardFrost(sf::Vector2f{float(BOARD_PIXEL_W), float(BOARD_PIXEL_H)});
                boardFrost.setPosition(sf::Vector2f{BOARD_X, BOARD_Y});
                boardFrost.setFillColor(sf::Color(100, 220, 255, 30));
                renderWindow.draw(boardFrost);

                // Layered glowing cyan borders
                for (int i = 0; i < 4; ++i) {
                    sf::RectangleShape glow(sf::Vector2f{BOARD_PIXEL_W - i * 2.f, BOARD_PIXEL_H - i * 2.f});
                    glow.setPosition(sf::Vector2f{BOARD_X + i, BOARD_Y + i});
                    glow.setFillColor(sf::Color::Transparent);
                    glow.setOutlineColor(sf::Color(100, 240, 255, static_cast<std::uint8_t>(200 - i * 45)));
                    glow.setOutlineThickness(1.5f);
                    renderWindow.draw(glow);
                }

                if (fontLoaded) {
                    char buf[32];
                    std::snprintf(buf, sizeof(buf), "FREEZE: %.1fs", gameState.timeStopTimer);
                    
                    sf::Text freezeTxt(font, buf, 22);
                    freezeTxt.setFillColor(sf::Color(130, 240, 255));
                    freezeTxt.setStyle(sf::Text::Bold);
                    
                    float pulse = 1.f + 0.05f * std::sin(gameState.timeStopTimer * 8.f);
                    freezeTxt.setScale({pulse, pulse});
                    
                    sf::FloatRect bounds = freezeTxt.getLocalBounds();
                    freezeTxt.setOrigin({bounds.size.x * 0.5f, bounds.size.y * 0.5f});
                    freezeTxt.setPosition({BOARD_X + BOARD_PIXEL_W * 0.5f, BOARD_Y + BOARD_PIXEL_H * 0.08f});
                    
                    sf::Text shadow = freezeTxt;
                    shadow.setFillColor(sf::Color(10, 30, 50, 200));
                    shadow.setPosition({BOARD_X + BOARD_PIXEL_W * 0.5f + 2.f, BOARD_Y + BOARD_PIXEL_H * 0.08f + 2.f});
                    renderWindow.draw(shadow);
                    
                    renderWindow.draw(freezeTxt);
                }
            }

            hud.renderLeftPanel(renderWindow, gameState, PALETTES[activePalette], powerSelectMode, hoveredPowerIdx);
            hud.renderSidePanel(renderWindow, gameState, PALETTES[activePalette]);

            if (powerSelectMode) {
                sf::RectangleShape dim(sf::Vector2f{float(BOARD_PIXEL_W), float(BOARD_PIXEL_H)});
                dim.setPosition(sf::Vector2f{BOARD_X, BOARD_Y});
                dim.setFillColor(sf::Color(0, 0, 0, 160));
                renderWindow.draw(dim);

                if (fontLoaded) {
                    sf::Text pt(font, "POWER SELECTION...", 20);
                    pt.setFillColor(sf::Color(100, 255, 100));
                    pt.setStyle(sf::Text::Bold);
                    auto b = pt.getLocalBounds();
                    pt.setPosition(sf::Vector2f{
                        BOARD_X + (BOARD_PIXEL_W - b.size.x) * 0.5f,
                        BOARD_Y + BOARD_PIXEL_H * 0.4f
                    });
                    renderWindow.draw(pt);
                }
            }

            if (oPlacementMode)
                hud.renderOPlacementHint(renderWindow);

            if (slotResultTimer > 0.f)
                hud.renderSlotResult(renderWindow, lastSlotResult, slotResultTimer);

            if (tokenMilestoneNotifTimer > 0.f)
                hud.renderTokenMilestoneNotif(renderWindow, tokenMilestoneNotifTimer);

            if (showPowerModal) {
                renderPowerModal(renderWindow);
            }

            if (showDebugMenu) {
                renderDebugMenu(renderWindow);
            }

            if (gameState.paused) {
                pauseMenu.render(renderWindow, font, PALETTES[activePalette], sfxEnabled, musicEnabled, activePalette,
                                 WIN_W, WIN_H);
            }

            if (showAscensionModal) {
                renderAscensionModal(renderWindow);
            }

            if (showGameOver) {
                renderGameOverScreen(renderWindow);
            }

            if (showVictory) {
                renderVictoryScreen(renderWindow);
            }
        }

    private:
        // ── Callback kurulumu (constructor ve restartGame'de ortak) ───────────────
        void setupCallbacks() {
            gameState.onCellCleared = [this](int col, int row, sf::Color color) {
                boardRenderer.triggerCellFlash(col, row, color);
            };

            gameState.onLinesCleared = [this](int n, std::vector<int> rows) {
                flashRows = rows;
                flashTimer = 0.f;
                flashDuration = 0.35f;
                addLineClearParticles(rows);

                if (!rows.empty()) {
                    float sumY = 0.f;
                    for (int r : rows) sumY += (BOARD_Y + r * CELL_SIZE + CELL_SIZE * 0.5f);
                    float centerY = sumY / rows.size();
                    boardRenderer.triggerLineClearEffect(centerY, PALETTES[activePalette].uiAccent);
                    boardRenderer.triggerLineClearScalePop(gameState.board);
                }

                if (n >= 4) {
                    audio.playSound("clear4");
                    trauma = std::min(1.f, trauma + 0.8f);
                } else {
                    audio.playSound("clear1");
                    trauma = std::min(1.f, trauma + 0.35f);
                }

                // Spawn score floating text
                float avgRowY = BOARD_Y + BOARD_PIXEL_H * 0.5f;
                if (!rows.empty()) {
                    float sum = 0.f;
                    for (int r : rows) sum += r;
                    avgRowY = BOARD_Y + (sum / rows.size()) * CELL_SIZE + CELL_SIZE * 0.5f;
                }
                std::string scoreStr = "+" + std::to_string(LINE_SCORES[std::min(n, 4)] * gameState.level) + " PTS";
                sf::Color scoreColor = (n >= 4) ? sf::Color::Cyan : sf::Color::White;
                spawnFloatingText({BOARD_X + BOARD_PIXEL_W * 0.5f, avgRowY}, scoreStr, scoreColor);
                if (n >= 4) {
                    spawnFloatingText({BOARD_X + BOARD_PIXEL_W * 0.5f, avgRowY - 25.f}, "TETRIS!", sf::Color::Yellow);
                }

                int cur = gameState.scoreTokenMilestone;
                if (cur > lastTokenMilestone) {
                    lastTokenMilestone = cur;
                    tokenMilestoneNotifTimer = 2.0f;
                    audio.playSound("token");

                    // Spawn golden token earned text
                    spawnFloatingText({BOARD_X + BOARD_PIXEL_W * 0.5f, BOARD_Y + BOARD_PIXEL_H * 0.3f}, "+1 TOKEN", sf::Color(255, 220, 50));
                }
            };

            gameState.onPieceLocked = [this]() {
                audio.playSound("land", 0.05f);
                trauma = std::min(1.f, trauma + 0.2f);

                auto cells = gameState.fallingPiece.cells();
                if (!cells.empty()) {
                    float sumX = 0.f, sumY = 0.f;
                    for (auto [r, c] : cells) {
                        sumX += (BOARD_X + c * CELL_SIZE + CELL_SIZE * 0.5f);
                        sumY += (BOARD_Y + r * CELL_SIZE + CELL_SIZE * 0.5f);
                    }
                    sf::Vector2f center(sumX / cells.size(), sumY / cells.size());
                    boardRenderer.triggerPlacementEffect(center, gameState.fallingPiece.color());
                    boardRenderer.triggerPlacementScalePop(cells, gameState.board);
                }
            };

            gameState.onBombExploded = [this](int col, int row) {
                sf::Vector2f pos = sf::Vector2f{
                    BOARD_X + col * CELL_SIZE + CELL_SIZE * 0.5f,
                    BOARD_Y + row * CELL_SIZE + CELL_SIZE * 0.5f
                };
                trauma = std::min(1.f, trauma + 0.5f);
                audio.playSound("clear4");
                addBombExplosionParticles(pos);
            };

            gameState.onGameOver = [this]() {
                showGameOver = true;
                playerName = "";
                audio.playSound("gameover");
                audio.stopMusic(2.f);
                saveStats();
            };

            gameState.onSlotResult = [this](SlotResult result) {
                lastSlotResult = result;
                audio.playSound("spin");
                addSpinSparks();
                addSpinSparks();

                // Spin animasyonunun süresi (SelectionBar'daki animasyon kaç saniyeyse ona göre ayarla, örn: 1.5 saniye)
                spinWaitTimer = 1.5f;
                isSpinning = true;
                isFree = false;

                if (result.hasCombo) {
                    pendingPower.type = result.power;
                }

                // Animasyon dönmeye başladığı an düşen parçayı havada dondur
                gameState.freezeFalling(true);
            };

            gameState.onFreeSlotResult = [this](FreeSlotResult result) {
                lastFreeSlotResult = result;
                audio.playSound("spin");
                selectionBar.FreeStartSpin(gameState);
                addSpinSparks();
                addSpinSparks();

                spinWaitTimer = 1.5f;
                isSpinning = true;
                isFree = true;

                gameState.freezeFalling(true);
            };

            pauseMenu.updateLayout(WIN_W, WIN_H);

            pauseMenu.onToggleSFX = [this]() {
                sfxEnabled = !sfxEnabled;
                settings->sfxEnabled = sfxEnabled; // YENİ: Ayarı kaydet
                audio.setVolume(engine::AudioBus::SFX, sfxEnabled ? 85.f : 0.f);
            };

            pauseMenu.onToggleMusic = [this]() {
                musicEnabled = !musicEnabled;
                settings->musicEnabled = musicEnabled; // YENİ: Ayarı kaydet
                audio.setVolume(engine::AudioBus::Music, musicEnabled ? 45.f : 0.f);
            };

            pauseMenu.onSelectPalette = [this](int idx) {
                applyPalette(idx);
                settings->activePalette = idx; // YENİ: Ayarı kaydet
            };

            pauseMenu.onResume = [this]() {
                gameState.paused = false;
            };

            pauseMenu.onMainMenu = [this]() {
                m_sceneManager->replace(std::make_unique<MainMenuScene>(window, settings));
            };

            // Başlangıç paletini uygula
            applyPalette(0);
        }

        // ── Sonraki parçanın düşeceği sütun — akan ok göstergesi ─────────────────
        void renderNextPieceArrow(sf::RenderWindow &renderWindow) {
            const Piece &nextP = gameState.nextPieces[0];
            int minC = 4, maxC = -1;
            for (auto [r, c]: nextP.localCells()) {
                minC = std::min(minC, c);
                maxC = std::max(maxC, c);
            }
            if (maxC < 0) return;

            int startCol = std::max(0, std::min(gameState.nextPieceCol + minC, BOARD_COLS - 1));
            int endCol = std::max(0, std::min(gameState.nextPieceCol + maxC, BOARD_COLS - 1));

            constexpr int ARROW_COUNT = 3;
            constexpr float ARROW_SPACING = 1.f / ARROW_COUNT;
            constexpr int ARROW_H = 10;
            constexpr int ARROW_W = CELL_SIZE - 6;
            float topY = BOARD_Y - 32.f;

            sf::ConvexShape arrowShape(3);

            for (int col = startCol; col <= endCol; ++col) {
                float cx = BOARD_X + col * CELL_SIZE + CELL_SIZE * 0.5f;

                for (int k = 0; k < ARROW_COUNT; ++k) {
                    float phase = std::fmod(arrowPhase + k * ARROW_SPACING, 1.f);
                    float travel = (BOARD_Y - 4.f) - topY;
                    float y = topY + phase * travel;
                    float fade = std::sin(phase * 3.14159f);
                    std::uint8_t alpha = static_cast<std::uint8_t>(fade * 200.f);

                    sf::Color c = nextP.color();
                    c.a = alpha;

                    arrowShape.setPoint(0, {cx - ARROW_W * 0.5f, y});
                    arrowShape.setPoint(1, {cx + ARROW_W * 0.5f, y});
                    arrowShape.setPoint(2, {cx, y + ARROW_H});
                    arrowShape.setFillColor(c);
                    renderWindow.draw(arrowShape);
                }

                sf::RectangleShape stripe({float(CELL_SIZE), float(BOARD_PIXEL_H)});
                stripe.setPosition({BOARD_X + col * CELL_SIZE, BOARD_Y});
                sf::Color sc = nextP.color();
                sc.a = 18;
                stripe.setFillColor(sc);
                renderWindow.draw(stripe);
            }
        }

        void handlePowerClick(sf::Vector2f mouse) {
            auto &powers = gameState.slotMachine.powers;
            float toggleY = BOARD_Y + BOARD_PIXEL_H - 36.f;

            if (mouse.x < LEFT_PANEL_W && mouse.y >= toggleY && mouse.y < toggleY + 30.f) {
                powerSelectMode = !powerSelectMode;
                gameState.freezeFalling(powerSelectMode);
                return;
            }

            for (int i = 0; i < static_cast<int>(powers.size()); ++i) {
                float y = BOARD_Y + 28.f + i * 44.f;
                if (mouse.x >= 4.f && mouse.x < LEFT_PANEL_W - 4.f &&
                    mouse.y >= y && mouse.y < y + 38.f) {
                    executePower(i, mouse);
                    powerSelectMode = false;
                    if (!oPlacementMode) gameState.freezeFalling(false);
                    return;
                }
            }
        }

        void handleLeftPanelToggle(sf::Vector2f mouse) {
            float toggleY = BOARD_Y + BOARD_PIXEL_H - 36.f;
            if (mouse.x < LEFT_PANEL_W && mouse.y >= toggleY && mouse.y < toggleY + 30.f) {
                powerSelectMode = !powerSelectMode;
                gameState.freezeFalling(powerSelectMode);
            }
        }

        // YENİ EKLENEN: Gücün asıl işini yapan çekirdek fonksiyon
        void applyPowerEffect(PowerType p) {
            switch (p) {
                case PowerType::I_Small: clearBestRow(1);
                    break;
                case PowerType::I_Big: clearBestRow(2);
                    break;
                case PowerType::O_Small: gameState.applyTimeStop(4.0f);
                    break;
                case PowerType::O_Big: gameState.applyTimeStop(8.0f);
                    break;
                case PowerType::T_Big: gameState.freeReroll(3);
                    break;
                case PowerType::L_Small: gameState.applyRotateFalling(1, true);
                    break;
                case PowerType::L_Big: gameState.applyRotateFalling(2, true);
                    break;
                case PowerType::J_Small: gameState.applyRotateFalling(1, false);
                    break;
                case PowerType::J_Big: gameState.applyRotateFalling(2, false);
                    break;
                case PowerType::S_Small: gameState.applyColumnLower(1, 1);
                    break;
                case PowerType::S_Big: gameState.applyColumnLower(2, 1);
                    break;
                case PowerType::SZ_Special: gameState.applyColumnLower(1, 2);
                    break;
                case PowerType::Z_Small: gameState.applyRandomBlockClear(1);
                    break;
                case PowerType::Z_Big: gameState.applyRandomBlockClear(3);
                    break;
                default: break;
            }

            // Güç kullanıldıktan sonra o anki parçayı en üste al
            gameState.fallingPiece.row = 0;
            gameState.fallAccum = 0.f;
        }

        // GÜNCELLENEN: Sol panelden güç tıklandığında çalışan fonksiyon
        void executePower(int index, sf::Vector2f /*mousePos*/) {
            PowerType p = gameState.slotMachine.powers[index].type;
            sf::Color powerColor = gameState.slotMachine.powers[index].color();
            std::string pName = gameState.slotMachine.powers[index].name();

            // Spawn large floating text notification
            std::string notifText = "";
            switch (p) {
                case PowerType::I_Small: notifText = "ROW CLEARED!"; break;
                case PowerType::I_Big: notifText = "2 ROWS CLEARED!"; break;
                case PowerType::O_Small: notifText = "TIME FROZEN 4s!"; break;
                case PowerType::O_Big: notifText = "TIME FROZEN 8s!"; break;
                case PowerType::T_Big: notifText = "FREE SELECTION SPIN!"; break;
                case PowerType::L_Small:
                case PowerType::L_Big:
                case PowerType::J_Small:
                case PowerType::J_Big: notifText = "PIECE ROTATED!"; break;
                case PowerType::S_Small: notifText = "COLUMN FLATTENED!"; break;
                case PowerType::S_Big: notifText = "2 COLUMNS FLATTENED!"; break;
                case PowerType::SZ_Special: notifText = "COLUMN FLATTENED -2!"; break;
                case PowerType::Z_Small: notifText = "1 BLOCK DETONATED!"; break;
                case PowerType::Z_Big: notifText = "3 BLOCKS DETONATED!"; break;
                default: notifText = "POWER ACTIVATED!"; break;
            }
            spawnFloatingText({BOARD_X + BOARD_PIXEL_W * 0.5f, BOARD_Y + BOARD_PIXEL_H * 0.45f}, notifText, powerColor, 26);

            // Listedeki gücü "kullanıldı" olarak işaretle/sil
            gameState.activatePower(index);

            addPowerBlastParticles(powerColor);
            applyPowerEffect(p); // Asıl etkiyi tetikle
        }

        void usePowerHotkey(int index) {
            bool isSlotMachineEnabled = true;
            if (gameState.gameMode == GameMode::Custom && !gameState.customSettings.slotMachineEnabled) {
                isSlotMachineEnabled = false;
            }
            if (!isSlotMachineEnabled) return;

            auto &powers = gameState.slotMachine.powers;
            if (index >= 0 && index < static_cast<int>(powers.size())) {
                executePower(index, {0.f, 0.f});
            }
        }

        void clearBestRow(int count) {
            std::vector<std::pair<int, int> > rowFill;
            for (int r = 0; r < BOARD_ROWS; ++r) {
                int filled = 0;
                for (int c = 0; c < BOARD_COLS; ++c)
                    if (!gameState.board.isEmpty(c, r)) filled++;
                if (filled > 0) rowFill.emplace_back(filled, r);
            }
            std::sort(rowFill.rbegin(), rowFill.rend());
            int cleared = std::min(count, static_cast<int>(rowFill.size()));
            std::vector<int> targetRows;
            for (int i = 0; i < cleared; ++i)
                targetRows.push_back(rowFill[i].second);
            std::sort(targetRows.begin(), targetRows.end());
            for (int r: targetRows)
                gameState.applyClearRow(r);
        }

        void drawTokenGlow(sf::RenderWindow &renderWindow) {
            auto cells = gameState.fallingPiece.cells();
            for (auto [r, c]: cells) {
                if (r < 0) continue;
                sf::CircleShape glow(CELL_SIZE * 0.6f);
                glow.setOrigin(sf::Vector2f{CELL_SIZE * 0.6f, CELL_SIZE * 0.6f});
                glow.setPosition(sf::Vector2f{
                    BOARD_X + c * CELL_SIZE + CELL_SIZE * 0.5f,
                    BOARD_Y + r * CELL_SIZE + CELL_SIZE * 0.5f
                });
                float pulse = 0.5f + 0.5f * std::sin(
                                  static_cast<float>(std::clock()) / 200.f);
                glow.setFillColor(sf::Color(255, 220, 50,
                                            static_cast<std::uint8_t>(60 + 80 * pulse)));
                renderWindow.draw(glow);
            }
        }

        void updateDragPreview() {
            if (!selectionBar.isDragging()) {
                boardRenderer.setDragPreview(std::nullopt, true);
                return;
            }
            const auto &drag = selectionBar.dragState();
            sf::FloatRect br = boardRenderer.boardRect();
            br.position.x -= 120.f;
            br.size.x += 240.f;

            if (!br.contains(drag.mousePos)) {
                boardRenderer.setDragPreview(std::nullopt, true);
                return;
            }
            int col = boardRenderer.screenXToCol(drag.mousePos.x);
            int row = static_cast<int>((drag.mousePos.y - BOARD_Y) / CELL_SIZE);
            auto [valid, placed] = gameState.board.findDropPosition(drag.piece, col, row);
            boardRenderer.setDragPreview(placed, valid);
        }

        void updateFlash(float dt) {
            if (flashDuration <= 0.f) return;
            flashTimer += dt;
            if (flashTimer >= flashDuration) {
                flashTimer = flashDuration = 0.f;
                flashRows.clear();
                boardRenderer.setFlashRows({}, 0.f);
            }
        }

        void addPowerBlastParticles(sf::Color c) {
            if (!particleSystem) return;
            engine::EmitterComponent config;
            config.speed = 450.f;
            config.speedSpread = 150.f;
            config.direction = 0.f;
            config.directionSpread = 180.f; // 360 degrees spread
            config.lifetime = 0.45f;
            config.lifetimeSpread = 0.15f;
            config.sizeStart = 8.f;
            config.sizeEnd = 0.f;
            config.colorStart = c;
            config.colorEnd = sf::Color(c.r, c.g, c.b, 0);
            config.gravityScale = 0.f; // No gravity

            particleSystem->burst(sf::Vector2f{BOARD_X + BOARD_PIXEL_W * 0.5f, BOARD_Y + BOARD_PIXEL_H * 0.5f},
                                  config, 60);
        }

        void addBombExplosionParticles(sf::Vector2f pos) {
            if (!particleSystem) return;

            // Fire particles: 25 orange/red particles, fast radial burst, rise slightly
            engine::EmitterComponent fireConfig;
            fireConfig.speed = 300.f;
            fireConfig.speedSpread = 100.f;
            fireConfig.direction = 0.f;
            fireConfig.directionSpread = 180.f;
            fireConfig.lifetime = 0.5f;
            fireConfig.lifetimeSpread = 0.15f;
            fireConfig.sizeStart = 10.f;
            fireConfig.sizeEnd = 0.f;
            fireConfig.colorStart = sf::Color(255, 120 + rand() % 80, 0, 240);
            fireConfig.colorEnd = sf::Color(180, 30, 0, 0);
            fireConfig.gravityScale = -0.2f;

            particleSystem->burst(pos, fireConfig, 25);

            // Smoke particles: 15 gray particles, slower, rises upward (negative gravity)
            engine::EmitterComponent smokeConfig;
            smokeConfig.speed = 100.f;
            smokeConfig.speedSpread = 50.f;
            smokeConfig.direction = -90.f;
            smokeConfig.directionSpread = 45.f;
            smokeConfig.lifetime = 0.9f;
            smokeConfig.lifetimeSpread = 0.3f;
            smokeConfig.sizeStart = 12.f;
            smokeConfig.sizeEnd = 4.f;
            smokeConfig.colorStart = sf::Color(100, 100, 110, 180);
            smokeConfig.colorEnd = sf::Color(60, 60, 60, 0);
            smokeConfig.gravityScale = -0.5f;

            particleSystem->burst(pos, smokeConfig, 15);
        }

        void addLineClearParticles(const std::vector<int> &rows) {
            if (!particleSystem) return;
            for (int r: rows) {
                for (int c = 0; c < BOARD_COLS; ++c) {
                    sf::Vector2f pos = sf::Vector2f{
                        BOARD_X + c * CELL_SIZE + CELL_SIZE * 0.5f,
                        BOARD_Y + r * CELL_SIZE + CELL_SIZE * 0.5f
                    };
                    sf::Color randColor(200 + rand() % 55, 200 + rand() % 55, 100 + rand() % 155, 255);
                    
                    engine::EmitterComponent config;
                    config.speed = 200.f;
                    config.speedSpread = 100.f;
                    config.direction = 0.f;
                    config.directionSpread = 180.f; // 360 degrees
                    config.lifetime = 0.6f;
                    config.lifetimeSpread = 0.2f;
                    config.sizeStart = 6.f;
                    config.sizeEnd = 2.f;
                    config.colorStart = randColor;
                    config.colorEnd = sf::Color(randColor.r, randColor.g, randColor.b, 0);
                    config.gravityScale = 0.8f; // downward fall

                    particleSystem->burst(pos, config, 5);
                }
            }
        }

        void addPlaceParticles(sf::Vector2f pos) {
            if (!particleSystem) return;
            sf::Color randColor(100 + rand() % 155, 200, 255, 220);

            engine::EmitterComponent config;
            config.speed = 100.f;
            config.speedSpread = 50.f;
            config.direction = -90.f; // Upward
            config.directionSpread = 45.f; // cone spread
            config.lifetime = 0.3f;
            config.lifetimeSpread = 0.1f;
            config.sizeStart = 4.f;
            config.sizeEnd = 0.f;
            config.colorStart = randColor;
            config.colorEnd = sf::Color(randColor.r, randColor.g, randColor.b, 0);
            config.gravityScale = 0.3f; // light downward drift

            particleSystem->burst(pos, config, 10);
        }

        void spawnFloatingText(sf::Vector2f pos, const std::string &str, sf::Color color, unsigned int fontSize = 18) {
            engine::Entity e = registry().createEntity();
            registry().addComponent<engine::TransformComponent>(e, engine::TransformComponent{pos});
            registry().addComponent<tetris::FloatingTextComponent>(e, tetris::FloatingTextComponent{str, color, 1.2f, 1.2f, {0.f, -60.f}, fontSize});
        }

        void addSpinSparks() {
            if (!particleSystem) return;
            for (int i = 0; i < 3; ++i) {
                sf::Vector2f pos = selectionBar.slotCenter(i);
                
                engine::EmitterComponent config;
                config.speed = 150.f;
                config.speedSpread = 50.f;
                config.direction = 0.f;
                config.directionSpread = 180.f; // 360 degrees spread
                config.lifetime = 0.35f;
                config.lifetimeSpread = 0.15f;
                config.sizeStart = 5.f;
                config.sizeEnd = 0.f;
                sf::Color c(255, 200 + rand() % 55, 50, 220); // Golden sparks
                config.colorStart = c;
                config.colorEnd = sf::Color(c.r, c.g, c.b, 0);
                config.gravityScale = 0.4f; // fall down slightly

                particleSystem->burst(pos, config, 3);
            }
        }

        void renderPowerModal(sf::RenderWindow &renderWindow) {
            // 1. Fullscreen dark dim overlay
            sf::RectangleShape dim(sf::Vector2f{float(WIN_W), float(WIN_H)});
            dim.setFillColor(sf::Color(0, 0, 0, 195));
            renderWindow.draw(dim);

            if (!fontLoaded) return;

            // 2. Balatro-style playing card dimensions & position
            float mw = 280.f, mh = 410.f;
            float mx = (WIN_W - mw) * 0.5f;
            float my = (WIN_H - mh) * 0.5f;

            sf::Color powerColor = pendingPower.color();

            // 3. Nested neon outer glow sways
            float timeSec = static_cast<float>(std::clock()) / 1000.f;
            float pulseGlow = 0.5f + 0.5f * std::sin(timeSec * 6.f);

            for (int i = 0; i < 4; ++i) {
                sf::RectangleShape glowBox(sf::Vector2f{mw + i * 4.f, mh + i * 4.f});
                glowBox.setOrigin({(mw + i * 4.f) * 0.5f, (mh + i * 4.f) * 0.5f});
                glowBox.setPosition({mx + mw * 0.5f, my + mh * 0.5f});
                glowBox.setFillColor(sf::Color::Transparent);
                
                sf::Color gc = powerColor;
                gc.a = static_cast<std::uint8_t>((40 - i * 10) * (0.8f + 0.2f * pulseGlow));
                glowBox.setOutlineColor(gc);
                glowBox.setOutlineThickness(1.5f);
                renderWindow.draw(glowBox);
            }

            // 4. Main Card Shape
            sf::RectangleShape card(sf::Vector2f{mw, mh});
            card.setPosition(sf::Vector2f{mx, my});
            card.setFillColor(sf::Color(15, 15, 26, 245));
            card.setOutlineColor(powerColor);
            card.setOutlineThickness(3.f);
            renderWindow.draw(card);

            // 5. Header Texts
            sf::Text title(font, "NEW POWER EARNED!", 13);
            title.setFillColor(sf::Color(255, 215, 0));
            title.setStyle(sf::Text::Bold);
            sf::FloatRect bTitle = title.getLocalBounds();
            title.setPosition({mx + (mw - bTitle.size.x) * 0.5f, my + 18.f});
            renderWindow.draw(title);

            sf::Text pName(font, pendingPower.name(), 18);
            pName.setFillColor(powerColor);
            pName.setStyle(sf::Text::Bold);
            sf::FloatRect bName = pName.getLocalBounds();
            pName.setPosition({mx + (mw - bName.size.x) * 0.5f, my + 40.f});
            renderWindow.draw(pName);

            // 6. Glowing seal background and Tetromino block graphic
            float sealRadius = 45.f;
            sf::CircleShape seal(sealRadius);
            seal.setOrigin({sealRadius, sealRadius});
            seal.setPosition({mx + mw * 0.5f, my + 135.f});
            seal.setFillColor(sf::Color(powerColor.r / 6, powerColor.g / 6, powerColor.b / 6, 120));
            seal.setOutlineColor(sf::Color(powerColor.r, powerColor.g, powerColor.b, 80));
            seal.setOutlineThickness(1.5f);
            renderWindow.draw(seal);

            // Resolve Tetromino Type of the power to draw
            TetrominoType drawType = TetrominoType::COUNT;
            switch (pendingPower.type) {
                case PowerType::I_Small:
                case PowerType::I_Big: drawType = TetrominoType::I; break;
                case PowerType::O_Small:
                case PowerType::O_Big: drawType = TetrominoType::O; break;
                case PowerType::T_Big: drawType = TetrominoType::T; break;
                case PowerType::L_Small:
                case PowerType::L_Big: drawType = TetrominoType::L; break;
                case PowerType::J_Small:
                case PowerType::J_Big: drawType = TetrominoType::J; break;
                case PowerType::S_Small:
                case PowerType::S_Big: drawType = TetrominoType::S; break;
                case PowerType::Z_Small:
                case PowerType::Z_Big: drawType = TetrominoType::Z; break;
                case PowerType::SZ_Special: drawType = TetrominoType::S; break;
                default: break;
            }

            if (drawType != TetrominoType::COUNT) {
                Piece p; p.type = drawType; p.rotation = 0;
                float bSize = 16.f;
                float sumC = 0.f, sumR = 0.f;
                auto cells = p.localCells();
                for (auto [r, c] : cells) { sumC += c; sumR += r; }
                sf::Vector2f tCenter(sumC / cells.size(), sumR / cells.size());
                
                sf::RectangleShape previewCell(sf::Vector2f{bSize - 2.f, bSize - 2.f});
                previewCell.setFillColor(powerColor);
                previewCell.setOutlineColor(sf::Color::White);
                previewCell.setOutlineThickness(-1.f);
                
                for (auto [r, c] : cells) {
                    float px_cell = mx + mw * 0.5f + (c - tCenter.x) * bSize - (bSize - 2.f) * 0.5f;
                    float py_cell = my + 135.f + (r - tCenter.y) * bSize - (bSize - 2.f) * 0.5f;
                    previewCell.setPosition({px_cell, py_cell});
                    renderWindow.draw(previewCell);
                }
            }

            // 7. Descriptions & Instructions
            sf::Text descTxt(font, pendingPower.description(), 11);
            descTxt.setFillColor(sf::Color(210, 210, 230));
            sf::FloatRect bDesc = descTxt.getLocalBounds();
            descTxt.setPosition({mx + (mw - bDesc.size.x) * 0.5f, my + 205.f});
            renderWindow.draw(descTxt);

            sf::Text hint1(font, "Use it immediately or save to lists.", 9);
            hint1.setFillColor(sf::Color(140, 140, 150));
            sf::FloatRect bHint1 = hint1.getLocalBounds();
            hint1.setPosition({mx + (mw - bHint1.size.x) * 0.5f, my + 230.f});
            renderWindow.draw(hint1);

            sf::Text hint2(font, "Hotkeys: Keys [1] - [5] (during play)", 9);
            hint2.setFillColor(sf::Color(255, 220, 50));
            sf::FloatRect bHint2 = hint2.getLocalBounds();
            hint2.setPosition({mx + (mw - bHint2.size.x) * 0.5f, my + 250.f});
            renderWindow.draw(hint2);

            // 8. Buttons
            sf::Vector2f mouse = sf::Vector2f(sf::Mouse::getPosition(window));
            mouse = window.mapPixelToCoords(sf::Mouse::getPosition(window));

            btnUseNowRect = sf::FloatRect({mx + 15.f, my + 300.f}, {115.f, 42.f});
            bool hoverUse = btnUseNowRect.contains(mouse);

            sf::RectangleShape btnUse(btnUseNowRect.size);
            btnUse.setPosition(btnUseNowRect.position);
            btnUse.setFillColor(hoverUse ? sf::Color(45, 150, 45) : sf::Color(30, 100, 30));
            btnUse.setOutlineColor(sf::Color(100, 255, 100));
            btnUse.setOutlineThickness(2.f);
            if (hoverUse) {
                btnUse.setScale({1.04f, 1.04f});
                btnUse.setOrigin(btnUseNowRect.size * 0.5f);
                btnUse.setPosition(btnUseNowRect.position + btnUseNowRect.size * 0.5f);
            }
            renderWindow.draw(btnUse);

            sf::Text txtUse(font, "USE NOW", 13);
            txtUse.setFillColor(sf::Color::White);
            txtUse.setStyle(sf::Text::Bold);
            sf::FloatRect b3 = txtUse.getLocalBounds();
            sf::Vector2f usePos = {
                btnUseNowRect.position.x + (btnUseNowRect.size.x - b3.size.x) * 0.5f,
                btnUseNowRect.position.y + (btnUseNowRect.size.y - b3.size.y) * 0.5f - 4.f
            };
            txtUse.setPosition(usePos);
            renderWindow.draw(txtUse);

            btnKeepRect = sf::FloatRect({mx + 150.f, my + 300.f}, {115.f, 42.f});
            bool hoverKeep = btnKeepRect.contains(mouse);

            sf::RectangleShape btnKeep(btnKeepRect.size);
            btnKeep.setPosition(btnKeepRect.position);
            btnKeep.setFillColor(hoverKeep ? sf::Color(80, 80, 120) : sf::Color(50, 50, 80));
            btnKeep.setOutlineColor(sf::Color(150, 150, 255));
            btnKeep.setOutlineThickness(2.f);
            if (hoverKeep) {
                btnKeep.setScale({1.04f, 1.04f});
                btnKeep.setOrigin(btnKeepRect.size * 0.5f);
                btnKeep.setPosition(btnKeepRect.position + btnKeepRect.size * 0.5f);
            }
            renderWindow.draw(btnKeep);

            sf::Text txtKeep(font, "KEEP", 13);
            txtKeep.setFillColor(sf::Color::White);
            txtKeep.setStyle(sf::Text::Bold);
            sf::FloatRect b4 = txtKeep.getLocalBounds();
            sf::Vector2f keepPos = {
                btnKeepRect.position.x + (btnKeepRect.size.x - b4.size.x) * 0.5f,
                btnKeepRect.position.y + (btnKeepRect.size.y - b4.size.y) * 0.5f - 4.f
            };
            txtKeep.setPosition(keepPos);
            renderWindow.draw(txtKeep);
        }

        void renderDebugMenu(sf::RenderWindow &renderWindow) {
            if (!fontLoaded) return;

            // Arka planı hafifçe karart
            sf::RectangleShape dim(sf::Vector2f{float(WIN_W), float(WIN_H)});
            dim.setFillColor(sf::Color(0, 0, 0, 200));
            renderWindow.draw(dim);

            // Başlık
            sf::Text title(font, ">> DEVELOPER TEST MENU << (Press F1 to close)", 18);
            title.setFillColor(sf::Color(100, 255, 150));
            title.setStyle(sf::Text::Bold);
            title.setPosition({60.f, 40.f});
            renderWindow.draw(title);

            // Grid Ayarları
            float startX = 60.f, startY = 80.f;
            float btnW = 180.f, btnH = 40.f;
            float spacingX = 15.f, spacingY = 15.f;
            sf::Vector2f mouse = sf::Vector2f(sf::Mouse::getPosition(window));

            for (int i = 0; i < 15; ++i) {
                int col = i % 3;
                int row = i / 3;

                sf::FloatRect btnRect({startX + col * (btnW + spacingX), startY + row * (btnH + spacingY)},
                                      {btnW, btnH});
                bool hover = btnRect.contains(mouse);

                PowerSlot dummySlot;
                dummySlot.type = static_cast<PowerType>(i);
                sf::Color pColor = dummySlot.color();

                sf::RectangleShape btn(btnRect.size);
                btn.setPosition(btnRect.position);
                btn.setFillColor(hover
                                     ? sf::Color(pColor.r / 3, pColor.g / 3, pColor.b / 3, 255)
                                     : sf::Color(30, 30, 40, 255));
                btn.setOutlineColor(pColor);
                btn.setOutlineThickness(hover ? 2.f : 1.f);
                renderWindow.draw(btn);

                sf::Text txt(font, dummySlot.name(), 12);
                txt.setFillColor(sf::Color::White);
                auto b = txt.getLocalBounds();
                txt.setPosition({
                    btnRect.position.x + (btnRect.size.x - b.size.x) * 0.5f,
                    btnRect.position.y + (btnRect.size.y - b.size.y) * 0.5f - 3.f
                });
                renderWindow.draw(txt);
            }
        }

        void restartGame() {
            gameState = GameState(static_cast<std::uint32_t>(std::time(nullptr)));
            lastTokenMilestone = 0;
            setupCallbacks();
            audio.resumeMusic();
            showGameOver = false;
            showVictory = false;
            showAscensionModal = false;
            m_statsSaved = false;
            m_newBest = false;
            m_newWorst = false;
            flashRows.clear();

            gameState.gameMode = settings->activeMode;
            if (gameState.gameMode == GameMode::GarbageClear) {
                gameState.setupGarbageBoard(settings->garbageRatio);
                gameState.countdownTimer = 0.f;
                gameState.fallInterval = 2.0f;
            } else if (gameState.gameMode == GameMode::GarbageTimeAttack) {
                gameState.setupGarbageBoard(settings->garbageRatio);
                gameState.countdownTimer = 240.f;
            } else if (gameState.gameMode == GameMode::ScoreTimeAttack) {
                gameState.countdownTimer = settings->scoreTimeLimit;
                gameState.targetScore = settings->scoreTimeTarget;
            } else if (gameState.gameMode == GameMode::Ascension) {
                gameState.countdownTimer = 90.f;
                gameState.targetScore = 1500;
            } else if (gameState.gameMode == GameMode::TacticalStep) {
                gameState.countdownTimer = 0.f;
            } else if (gameState.gameMode == GameMode::TacticalGarbageClear) {
                gameState.setupGarbageBoard(settings->garbageRatio);
                gameState.countdownTimer = 0.f;
            } else if (gameState.gameMode == GameMode::Custom) {
                gameState.customSettings = settings->customSettings;
                if (gameState.customSettings.garbageBoardEnabled) {
                    gameState.setupGarbageBoard(gameState.customSettings.garbageRatio);
                } else {
                    gameState.board.clear();
                }
                if (gameState.customSettings.timeLimitEnabled) {
                    gameState.countdownTimer = gameState.customSettings.timeLimit;
                } else {
                    gameState.countdownTimer = 0.f;
                }
                if (gameState.customSettings.targetScoreEnabled) {
                    gameState.targetScore = gameState.customSettings.targetScore;
                } else {
                    gameState.targetScore = 0;
                }
                if (gameState.customSettings.gravityEnabled) {
                    gameState.fallInterval = 1.0f;
                } else {
                    gameState.fallInterval = 999999.f;
                }
            } else { // Classic
                gameState.countdownTimer = 0.f;
            }

            gameState.onGameWon = [this]() {
                showVictory = true;
                playerName = "";
                audio.playSound("levelup");
                audio.stopMusic(2.f);
                saveStats();
            };
            
            // Reset particle system
            particleSystem = std::make_unique<engine::ParticleSystem>(registry());

            // Clear any floating texts
            auto floatSig = registry().buildSignature<engine::TransformComponent, tetris::FloatingTextComponent>();
            for (engine::Entity e : registry().view(floatSig)) {
                registry().destroyEntity(e);
            }

            flashTimer = flashDuration = 0.f;
            powerSelectMode = false;
        }

        void handleGameOverEvent(const sf::Event &event) {
            // Mouse Tıklamaları
            sf::Vector2f mouse = window.mapPixelToCoords(sf::Mouse::getPosition(window));
            if (const auto *e = event.getIf<sf::Event::MouseButtonPressed>()) {
                if (e->button == sf::Mouse::Button::Left) {
                    // TEKRAR OYNA
                    if (btnPlayAgainRect.contains(mouse)) {
                        restartGame();
                    }
                    // MAIN MENU
                    else if (btnMenuRect.contains(mouse)) {
                        m_sceneManager->replace(std::make_unique<MainMenuScene>(window, settings));
                    }
                }
            }
        }

        void renderEndGameScreen(sf::RenderWindow &renderWindow, const std::string &title, sf::Color titleColor) {
            if (!fontLoaded) return;

            // Arka planı karart
            sf::RectangleShape dim({float(WIN_W), float(WIN_H)});
            dim.setFillColor(sf::Color(0, 0, 0, 230));
            renderWindow.draw(dim);

            auto drawText = [&](const std::string &str, float x, float y, int size, sf::Color col, bool bold = false) {
                sf::Text t(font, str, size);
                t.setFillColor(col);
                if (bold) t.setStyle(sf::Text::Bold);
                sf::FloatRect b = t.getLocalBounds();
                t.setPosition({x - b.size.x / 2.f, y});
                renderWindow.draw(t);
            };

            float cx = WIN_W / 2.f;

            // Draw Pulsating Title
            float timeSec = static_cast<float>(std::clock()) / 1000.f;
            float pulse = 1.f + 0.05f * std::sin(timeSec * 5.f);
            sf::Text tTitle(font, title, 50);
            tTitle.setFillColor(titleColor);
            tTitle.setStyle(sf::Text::Bold);
            tTitle.setScale({pulse, pulse});
            sf::FloatRect bTitle = tTitle.getLocalBounds();
            tTitle.setOrigin({bTitle.position.x + bTitle.size.x / 2.f, bTitle.position.y + bTitle.size.y / 2.f});
            tTitle.setPosition({cx, 130.f});
            renderWindow.draw(tTitle);

            // Mode Name
            drawText(getModeName(gameState.gameMode), cx, 185.f, 20, sf::Color::Cyan, true);

            // Record Banner
            bool isScoreBased = (gameState.gameMode == GameMode::Classic ||
                                 gameState.gameMode == GameMode::ScoreTimeAttack ||
                                 gameState.gameMode == GameMode::Ascension ||
                                 gameState.gameMode == GameMode::TacticalStep);

            if (m_newBest) {
                sf::Color goldPulseColor(255, static_cast<std::uint8_t>(200 + 55 * std::sin(timeSec * 6.f)), 0);
                std::string bestMsg = isScoreBased ? "NEW HIGH SCORE!" : "NEW BEST PERFORMANCE!";
                drawText(bestMsg, cx, 225.f, 20, goldPulseColor, true);
            } else if (m_newWorst) {
                sf::Color redPulseColor(255, static_cast<std::uint8_t>(100 + 50 * std::sin(timeSec * 6.f)), static_cast<std::uint8_t>(100 + 50 * std::sin(timeSec * 6.f)));
                drawText("NEW WORST PERFORMANCE...", cx, 225.f, 18, redPulseColor, true);
            }

            // Stats Box
            float boxW = 400.f, boxH = 170.f;
            float boxY = 270.f;
            sf::RectangleShape statsBox({boxW, boxH});
            statsBox.setPosition({cx - boxW / 2.f, boxY});
            statsBox.setFillColor(sf::Color(15, 15, 25, 230));
            statsBox.setOutlineColor(PALETTES[activePalette].panelBorder);
            statsBox.setOutlineThickness(1.5f);
            renderWindow.draw(statsBox);

            drawText("CURRENT PERFORMANCE: " + m_performanceStr, cx, boxY + 15.f, 15, sf::Color::White, true);
            drawText("BEST RECORD: " + m_prevBestStr, cx, boxY + 50.f, 15, sf::Color(100, 255, 100));
            drawText("WORST RECORD: " + m_prevWorstStr, cx, boxY + 85.f, 15, sf::Color(255, 100, 100));
            drawText("GAMES PLAYED: " + std::to_string(m_playCount), cx, boxY + 125.f, 14, sf::Color(160, 160, 160));

            // Buttons
            sf::Vector2f mouse = window.mapPixelToCoords(sf::Mouse::getPosition(window));

            auto drawBtn = [&](sf::FloatRect &rect, float y, const std::string &text, sf::Color baseCol) {
                rect = sf::FloatRect({cx - 120.f, y}, {240.f, 40.f});
                bool hover = rect.contains(mouse);

                sf::RectangleShape rs(rect.size);
                rs.setPosition(rect.position);
                rs.setFillColor(hover ? baseCol : sf::Color(baseCol.r / 2, baseCol.g / 2, baseCol.b / 2));
                rs.setOutlineThickness(1.5f);
                rs.setOutlineColor(sf::Color::White);
                renderWindow.draw(rs);

                drawText(text, cx, y + 8.f, 16, sf::Color::White, true);
            };

            drawBtn(btnPlayAgainRect, 470.f, "PLAY AGAIN", sf::Color(150, 100, 40));
            drawBtn(btnMenuRect, 530.f, "MAIN MENU", sf::Color(80, 80, 100));
        }

        void renderGameOverScreen(sf::RenderWindow &renderWindow) {
            renderEndGameScreen(renderWindow, "GAME OVER", PALETTES[activePalette].uiAccent);
        }

        void renderAscensionModal(sf::RenderWindow &renderWindow) {
            // 1. Fullscreen dark dim overlay
            sf::RectangleShape dim(sf::Vector2f{float(WIN_W), float(WIN_H)});
            dim.setFillColor(sf::Color(0, 0, 0, 205));
            renderWindow.draw(dim);

            if (!fontLoaded) return;

            // Header Texts
            sf::Text title(font, "CHOOSE AN ASCENSION POWER!", 22);
            title.setFillColor(sf::Color(255, 215, 0));
            title.setStyle(sf::Text::Bold);
            sf::FloatRect bTitle = title.getLocalBounds();
            title.setOrigin({bTitle.position.x + bTitle.size.x / 2.f, bTitle.position.y + bTitle.size.y / 2.f});
            title.setPosition({WIN_W * 0.5f, 100.f});
            renderWindow.draw(title);

            sf::Text subtitle(font, "Target score reached! Choose a power to extend time.", 13);
            subtitle.setFillColor(sf::Color(200, 200, 220));
            sf::FloatRect bSub = subtitle.getLocalBounds();
            subtitle.setOrigin({bSub.position.x + bSub.size.x / 2.f, bSub.position.y + bSub.size.y / 2.f});
            subtitle.setPosition({WIN_W * 0.5f, 135.f});
            renderWindow.draw(subtitle);

            // Draw 3 cards side-by-side
            float cardW = 160.f, cardH = 250.f;
            float startY = 220.f;
            sf::Vector2f mouse = sf::Vector2f(sf::Mouse::getPosition(window));
            mouse = window.mapPixelToCoords(sf::Mouse::getPosition(window));

            float timeSec = static_cast<float>(std::clock()) / 1000.f;

            for (int i = 0; i < 3; ++i) {
                float cardX = 55.f + i * (cardW + 55.f);
                ascensionCardRects[i] = sf::FloatRect({cardX, startY}, {cardW, cardH});

                bool hovered = ascensionCardRects[i].contains(mouse);

                PowerType p = ascensionChoices[i];
                PowerSlot dummySlot;
                dummySlot.type = p;

                sf::Color pColor = dummySlot.color();

                // Pulse glow for hovered card
                float scale = hovered ? 1.05f : 1.0f;
                float tiltAngle = std::sin(timeSec * 3.f + i * 1.5f) * 1.5f;

                sf::Transform transform;
                transform.translate({cardX + cardW * 0.5f, startY + cardH * 0.5f});
                transform.rotate(sf::degrees(tiltAngle));
                transform.scale({scale, scale});

                // Neon Outer Glow for hovered card
                if (hovered) {
                    float pulse = 0.5f + 0.5f * std::sin(timeSec * 8.f);
                    for (int j = 0; j < 3; ++j) {
                        sf::RectangleShape glowBox(sf::Vector2f{cardW + j * 4.f, cardH + j * 4.f});
                        glowBox.setOrigin(glowBox.getSize() * 0.5f);
                        glowBox.setFillColor(sf::Color::Transparent);
                        sf::Color gc = pColor;
                        gc.a = static_cast<std::uint8_t>((50 - j * 15) * (0.8f + 0.2f * pulse));
                        glowBox.setOutlineColor(gc);
                        glowBox.setOutlineThickness(1.5f);
                        renderWindow.draw(glowBox, transform);
                    }
                }

                // Card body
                sf::RectangleShape card(sf::Vector2f{cardW, cardH});
                card.setOrigin(card.getSize() * 0.5f);
                card.setFillColor(sf::Color(15, 15, 26, 245));
                card.setOutlineColor(hovered ? pColor : sf::Color(80, 80, 100));
                card.setOutlineThickness(hovered ? 3.f : 1.5f);
                renderWindow.draw(card, transform);

                // Draw a colored top bar inside the card
                sf::RectangleShape topBar({cardW - 16.f, 6.f});
                topBar.setOrigin(topBar.getSize() * 0.5f);
                topBar.setPosition({0.f, -cardH * 0.5f + 16.f});
                topBar.setFillColor(pColor);
                renderWindow.draw(topBar, transform);

                // Power Name
                sf::Text nameTxt(font, dummySlot.name(), 12);
                nameTxt.setFillColor(pColor);
                nameTxt.setStyle(sf::Text::Bold);
                sf::FloatRect bName = nameTxt.getLocalBounds();
                nameTxt.setOrigin({bName.position.x + bName.size.x / 2.f, bName.position.y + bName.size.y / 2.f});
                nameTxt.setPosition({0.f, -cardH * 0.5f + 40.f});
                renderWindow.draw(nameTxt, transform);

                // Power Description
                sf::Text descTxt(font, dummySlot.description(), 9);
                descTxt.setFillColor(sf::Color(200, 200, 220));
                sf::FloatRect bDesc = descTxt.getLocalBounds();
                descTxt.setOrigin({bDesc.position.x + bDesc.size.x / 2.f, bDesc.position.y + bDesc.size.y / 2.f});
                descTxt.setPosition({0.f, 20.f});
                renderWindow.draw(descTxt, transform);

                // Draw small icon/shape seal in card center
                float sealRadius = 25.f;
                sf::CircleShape seal(sealRadius);
                seal.setOrigin({sealRadius, sealRadius});
                seal.setPosition({0.f, -30.f});
                seal.setFillColor(sf::Color(pColor.r / 6, pColor.g / 6, pColor.b / 6, 120));
                seal.setOutlineColor(sf::Color(pColor.r, pColor.g, pColor.b, 80));
                seal.setOutlineThickness(1.f);
                renderWindow.draw(seal, transform);

                // Draw miniature block in the center
                TetrominoType drawType = TetrominoType::COUNT;
                switch (p) {
                    case PowerType::I_Small:
                    case PowerType::I_Big: drawType = TetrominoType::I; break;
                    case PowerType::O_Small:
                    case PowerType::O_Big: drawType = TetrominoType::O; break;
                    case PowerType::T_Big: drawType = TetrominoType::T; break;
                    case PowerType::L_Small:
                    case PowerType::L_Big: drawType = TetrominoType::L; break;
                    case PowerType::J_Small:
                    case PowerType::J_Big: drawType = TetrominoType::J; break;
                    case PowerType::S_Small:
                    case PowerType::S_Big: drawType = TetrominoType::S; break;
                    case PowerType::Z_Small:
                    case PowerType::Z_Big: drawType = TetrominoType::Z; break;
                    case PowerType::SZ_Special: drawType = TetrominoType::S; break;
                    default: break;
                }

                if (drawType != TetrominoType::COUNT) {
                    Piece pic; pic.type = drawType; pic.rotation = 0;
                    float bSize = 10.f;
                    float sumC = 0.f, sumR = 0.f;
                    auto cells = pic.localCells();
                    for (auto [r, c] : cells) { sumC += c; sumR += r; }
                    sf::Vector2f tCenter(sumC / cells.size(), sumR / cells.size());
                    
                    sf::RectangleShape previewCell(sf::Vector2f{bSize - 1.f, bSize - 1.f});
                    previewCell.setOrigin({(bSize - 1.f) * 0.5f, (bSize - 1.f) * 0.5f});
                    previewCell.setFillColor(pColor);
                    
                    for (auto [r, c] : cells) {
                        previewCell.setPosition({(c - tCenter.x) * bSize, -30.f + (r - tCenter.y) * bSize});
                        renderWindow.draw(previewCell, transform);
                    }
                } else if (p == PowerType::ExtraTime) {
                    sf::Text timeSym(font, "+45s", 18);
                    timeSym.setFillColor(pColor);
                    timeSym.setStyle(sf::Text::Bold);
                    sf::FloatRect bSym = timeSym.getLocalBounds();
                    timeSym.setOrigin({bSym.position.x + bSym.size.x / 2.f, bSym.position.y + bSym.size.y / 2.f});
                    timeSym.setPosition({0.f, -30.f});
                    renderWindow.draw(timeSym, transform);
                }
            }
        }

        void renderVictoryScreen(sf::RenderWindow &renderWindow) {
            renderEndGameScreen(renderWindow, "VICTORY!", sf::Color(255, 215, 0));
        }

        void openAscensionModal() {
            showAscensionModal = true;
            gameState.freezeFalling(true);
            
            std::vector<PowerType> allPowers;
            // Add all available power types (up to ExtraTime, which is 14)
            for (int i = 0; i <= 14; ++i) {
                allPowers.push_back(static_cast<PowerType>(i));
            }
            // Shuffle to get 3 random distinct ones
            std::shuffle(allPowers.begin(), allPowers.end(), std::mt19937(std::random_device{}()));
            
            ascensionChoices[0] = allPowers[0];
            ascensionChoices[1] = allPowers[1];
            ascensionChoices[2] = allPowers[2];
        }

        void saveStats() {
            if (m_statsSaved) return;
            m_statsSaved = true;

            int currentVal = 0;
            bool isScoreBased = (gameState.gameMode == GameMode::Classic ||
                                 gameState.gameMode == GameMode::ScoreTimeAttack ||
                                 gameState.gameMode == GameMode::Ascension ||
                                 gameState.gameMode == GameMode::TacticalStep ||
                                 gameState.gameMode == GameMode::Custom);

            if (gameState.gameMode == GameMode::Classic || gameState.gameMode == GameMode::Ascension || gameState.gameMode == GameMode::TacticalStep || gameState.gameMode == GameMode::Custom) {
                currentVal = gameState.score;
            } else if (gameState.gameMode == GameMode::ScoreTimeAttack) {
                if (gameState.gameWon) {
                    currentVal = 10000 + static_cast<int>(gameState.countdownTimer * 10.f);
                } else {
                    currentVal = gameState.score;
                }
            } else if (gameState.gameMode == GameMode::TacticalGarbageClear || gameState.gameMode == GameMode::GarbageClear) {
                if (gameState.gameWon) {
                    currentVal = static_cast<int>(gameState.countdownTimer);
                } else {
                    int remainingBlocks = 0;
                    if (gameState.gameMode == GameMode::TacticalGarbageClear) {
                        remainingBlocks = gameState.board.countOriginalGarbage();
                    } else {
                        for (int r = 0; r < BOARD_ROWS; ++r) {
                            for (int c = 0; c < BOARD_COLS; ++c) {
                                if (!gameState.board.isEmpty(c, r)) {
                                    remainingBlocks++;
                                }
                            }
                        }
                    }
                    currentVal = 5000 + remainingBlocks;
                }
            } else { // GarbageTimeAttack
                if (gameState.gameWon) {
                    currentVal = static_cast<int>(240.f - gameState.countdownTimer);
                } else {
                    int remainingBlocks = 0;
                    for (int r = 0; r < BOARD_ROWS; ++r) {
                        for (int c = 0; c < BOARD_COLS; ++c) {
                            if (!gameState.board.isEmpty(c, r)) {
                                remainingBlocks++;
                            }
                        }
                    }
                    currentVal = 5000 + remainingBlocks;
                }
            }

            auto stats = StatsManager::load("assets/stats.csv");
            auto& modeStats = stats[gameState.gameMode];

            int oldBest = -1;
            int oldWorst = -1;
            if (modeStats.playCount > 0 && !modeStats.history.empty()) {
                oldBest = modeStats.history[0];
                oldWorst = modeStats.history[0];
                for (int val : modeStats.history) {
                    if (isScoreBased) {
                        if (val > oldBest) oldBest = val;
                        if (val < oldWorst) oldWorst = val;
                    } else {
                        if (val < oldBest) oldBest = val;
                        if (val > oldWorst) oldWorst = val;
                    }
                }
            }

            // Determine if new best or new worst
            if (modeStats.playCount == 0) {
                m_newBest = true;
                m_newWorst = false;
            } else {
                if (isScoreBased) {
                    if (currentVal > oldBest) m_newBest = true;
                    if (currentVal < oldWorst) m_newWorst = true;
                } else {
                    if (currentVal < oldBest) m_newBest = true;
                    if (currentVal > oldWorst) m_newWorst = true;
                }
            }

            // Record old values for end screen display
            if (modeStats.playCount > 0) {
                m_prevBestStr = StatsManager::formatValue(gameState.gameMode, oldBest);
                m_prevWorstStr = StatsManager::formatValue(gameState.gameMode, oldWorst);
            } else {
                m_prevBestStr = "N/A";
                m_prevWorstStr = "N/A";
            }

            // Add new record and save
            StatsManager::addRecord(stats, gameState.gameMode, currentVal);
            StatsManager::save(stats, "assets/stats.csv");

            m_playCount = stats[gameState.gameMode].playCount;
            m_performanceStr = StatsManager::formatValue(gameState.gameMode, currentVal);
        }
 
        void chooseAscensionPower(int index) {
            PowerType chosen = ascensionChoices[index];
            
            if (chosen == PowerType::ExtraTime) {
                gameState.countdownTimer = 135.f;
                audio.playSound("token");
                spawnFloatingText({BOARD_X + BOARD_PIXEL_W * 0.5f, BOARD_Y + BOARD_PIXEL_H * 0.3f}, "+45 SECONDS!", sf::Color::Magenta);
            } else {
                gameState.slotMachine.awardPower(chosen);
                gameState.countdownTimer = 90.f;
                audio.playSound("token");
                spawnFloatingText({BOARD_X + BOARD_PIXEL_W * 0.5f, BOARD_Y + BOARD_PIXEL_H * 0.3f}, "POWER ACQUIRED!", sf::Color::Green);
            }
            
            // Increase target score progressively
            int increment = 1500 + 500 * gameState.level * gameState.level;
            gameState.targetScore += increment;
            
            // Increment level
            gameState.level += 1;
            gameState.recalcLevel();
            
            showAscensionModal = false;
            gameState.freezeFalling(false);
        }

        void applyPalette(int index) {
            activePalette = index;
            const auto &p = PALETTES[index];
            boardRenderer.setColors(p.boardBg, p.gridLines, p.panelBorder);
            // Tetromino renkleri render sırasında getPieceColor ile çözülecek
        }

        // ── Member değişkenler ────────────────────────────────────────────────────
        sf::RenderWindow &window;
        GameState gameState;
        BoardRenderer boardRenderer;
        SelectionBar selectionBar;
        engine::AudioManager audio;
        sf::Font font;
        bool fontLoaded{false};
        bool showGameOver{false};
        float trauma{0.f};

        // Flash
        std::vector<int> flashRows;
        float flashTimer{0.f}, flashDuration{0.f};

        // Slot machine sonuç gösterimi
        SlotResult lastSlotResult;
        float slotResultTimer{0.f};

        FreeSlotResult lastFreeSlotResult;
        float freeSlotResultTimer{0.f};

        bool isFree{false};

        // Güç seçim modu
        bool powerSelectMode{false};
        int selectedPowerIdx{-1};
        int hoveredPowerIdx{-1};

        // O_Big: düşen parçayı dondurdu, oyuncu board'a tıklayacak
        bool oPlacementMode{false};

        // Sonraki parça ok animasyonu için zaman fazı
        float arrowPhase{0.f};

        // 1000 puan jeton bildirimi
        float tokenMilestoneNotifTimer{0.f};
        int lastTokenMilestone{0};

        // Modal State
        bool showPowerModal{false};
        PowerSlot pendingPower; // Gösterilecek güç
        sf::FloatRect btnUseNowRect; // Şimdi kullan butonu çarpışma alanı
        sf::FloatRect btnKeepRect; // Sakla butonu çarpışma alanı

        // Spin Animasyonu Bekleme
        float spinWaitTimer{0.f};
        bool isSpinning{false};

        // Debug Menü Bayrağı
        bool showDebugMenu{false};

        std::unique_ptr<engine::ParticleSystem> particleSystem;

        // --- Yeni Sistemler ---
        bool sfxEnabled{true};
        bool musicEnabled{true};
        int activePalette{0};
        PauseMenu pauseMenu;

        TetrisHUD hud;

        AppSettings *settings;

        // Game Over / Score State
        std::string playerName{"AAA"};
        sf::FloatRect btnPlayAgainRect;
        sf::FloatRect btnMenuRect;

        // Auto-save & stats display states
        bool m_statsSaved{false};
        bool m_newBest{false};
        bool m_newWorst{false};
        std::string m_performanceStr;
        std::string m_prevBestStr;
        std::string m_prevWorstStr;
        int m_playCount{0};

        // Mouse Tilt Parallax juice
        sf::Vector2f m_mouseTilt{0.f, 0.f};
        float m_tiltRotation{0.f};

        // Game Mode Overlays
        bool showVictory{false};
        bool showAscensionModal{false};
        std::array<PowerType, 3> ascensionChoices;
        std::array<sf::FloatRect, 3> ascensionCardRects;
    };
} // namespace tetris
