//
// Created by e-r-e on 5/20/2026.
//

#ifndef SIMPLEENGINE2D_MAINMENUSCENE_HPP
#define SIMPLEENGINE2D_MAINMENUSCENE_HPP

#endif //SIMPLEENGINE2D_MAINMENUSCENE_HPP
#pragma once
#include "tetris/TetrisScene.hpp"
#include "tetris/AppSettings.hpp"
#include "tetris/ScoreManager.hpp"
#include <SFML/Graphics.hpp>
#include <vector>
#include <functional>
#include <string>

namespace tetris {

    class MainMenuScene : public engine::Scene {
    public:
        MainMenuScene(sf::RenderWindow& win, AppSettings* appSettings,
                      std::function<void()> startCallback,
                      std::function<void()> quitCallback);

        void onEnter() override;
        void onExit() override;
        void handleEvent(const sf::Event& event) override;
        void update(float dt, engine::InputManager& input) override;
        void render(sf::RenderWindow& window, float alpha) override;

    private:
        sf::RenderWindow& window;
        AppSettings*      settings;

        std::function<void()> onStartGame;
        std::function<void()> onQuitGame;

        sf::Font font;
        bool fontLoaded{false};

        sf::FloatRect btnNewGame;
        sf::FloatRect btnHighScores;
        sf::FloatRect btnQuit;
        sf::FloatRect btnSfx;
        sf::FloatRect btnMusic;
        std::vector<sf::FloatRect> paletteBtns;

        int hoveredBtn{-1}; // 0: New Game, 1: High Scores, 2: Quit, 3: SFX, 4: Music, 5-8: Palettes

        bool showScoreTable{false};
        std::vector<ScoreManager::Entry> highScores;
        sf::FloatRect btnBack;
        bool backHovered{false};

        void setupLayout();
        void renderScoreTable(sf::RenderWindow& window);
    };

} // namespace tetris