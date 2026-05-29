//
// Created by e-r-e on 5/20/2026.
//

#ifndef SIMPLEENGINE2D_MAINMENUSCENE_HPP
#define SIMPLEENGINE2D_MAINMENUSCENE_HPP

#endif //SIMPLEENGINE2D_MAINMENUSCENE_HPP
#pragma once
#include "../SceneManager.hpp"
#include "tetris/StatsManager.hpp"
#include <SFML/Graphics.hpp>
#include <map>

namespace tetris {

    class MainMenuScene : public engine::Scene {
    public:
        MainMenuScene(sf::RenderWindow& win, AppSettings* appSettings);

        void onEnter() override;
        void onExit() override;
        void handleEvent(const sf::Event& event) override;
        void update(float dt, engine::InputManager& input) override;
        void render(sf::RenderWindow& window, float alpha) override;

    private:
        sf::RenderWindow& window;
        AppSettings*      settings;

        sf::Font font;
        bool fontLoaded{false};

        sf::FloatRect btnNewGame;
        sf::FloatRect btnHighScores;
        sf::FloatRect btnQuit;
        sf::FloatRect btnSfx;
        sf::FloatRect btnMusic;
        std::vector<sf::FloatRect> paletteBtns;

        int hoveredBtn{-1}; // 0: New Game, 1: High Scores, 2: Quit, 3: SFX, 4: Music, 5-8: Palettes
        std::vector<float> btnScales;

        bool showScoreTable{false};
        std::map<GameMode, StatsManager::ModeStats> m_gameStats;
        sf::FloatRect btnBack;
        bool backHovered{false};

        // Game Mode Submenu
        bool m_showModeSelect{false};
        std::vector<sf::FloatRect> m_modeBtns;
        sf::FloatRect m_btnBackModeSelect;

        // Score table navigation
        GameMode m_scoreTableMode{GameMode::Classic};
        sf::FloatRect btnScorePrev;
        sf::FloatRect btnScoreNext;
        bool prevScoreHovered{false};
        bool nextScoreHovered{false};

        // Garbage Ratio Selection Modal
        bool m_showGarbageSelect{false};
        GameMode m_pendingMode{GameMode::Classic};
        sf::FloatRect btnRatioOpt1;
        sf::FloatRect btnRatioOpt2;
        sf::FloatRect btnRatioBack;
        int hoveredRatioBtn{-1};

        // Custom Mode Settings Menu
        struct CustomConfigUI {
            sf::FloatRect btnGravity;
            sf::FloatRect btnTactical;
            sf::FloatRect btnSlot;
            sf::FloatRect btnAscension;
            sf::FloatRect btnGarbage;
            sf::FloatRect btnGarbageRatio;
            sf::FloatRect btnSpecial;
            sf::FloatRect btnScore;
            sf::FloatRect btnTime;
            sf::FloatRect btnStart;
            sf::FloatRect btnBack;
        };
        bool m_showCustomConfig{false};
        CustomConfigUI m_customUI;
        int hoveredCustomBtn{-1};

        // Score Time Selection Modal
        bool m_showScoreTimeSelect{false};
        sf::FloatRect btnScoreTimeOpt1;
        sf::FloatRect btnScoreTimeOpt2;
        sf::FloatRect btnScoreTimeOpt3;
        sf::FloatRect btnScoreTimeBack;
        int hoveredScoreTimeBtn{-1};

        // UI Animation & Juice Variables
        float m_introTimer{0.f};
        float m_elapsedTime{0.f};
        float m_scoreTableTimer{0.f};
        sf::Vector2f m_mouseTilt{0.f, 0.f};
        int m_clickedBtn{-1};
        float m_clickTimer{0.f};

        struct MenuParticle {
            sf::Vector2f position;
            sf::Vector2f velocity;
            sf::Color color;
            float age{0.f};
            float lifetime{1.f};
            float size{4.f};
        };
        std::vector<MenuParticle> m_particles;

        void setupLayout();
        void renderScoreTable(sf::RenderWindow& window, const sf::Transform& transform);
    };

} // namespace tetris