#include "GameLoop.hpp"
#include "InputManager.hpp"
#include "tetris/TetrisScene.hpp"
#include "tetris/MainMenuScene.hpp"
#include "tetris/AppSettings.hpp"
#include "WindowUtils.hpp"
#include <SFML/Graphics.hpp>
#include <iostream>
#include <memory>
#include <functional>

int main() {
    try {
        AppSettings settings;

        sf::RenderWindow window{sf::VideoMode({tetris::WIN_W, tetris::WIN_H}), "Tetris", sf::Style::Default, sf::State::Windowed};
        window.setFramerateLimit(60);

        updateViewport(window, tetris::WIN_W, tetris::WIN_H);

        engine::InputManager input;
        engine::GameLoop loop(window, 1.f / 60.f);

        std::unique_ptr<engine::Scene> currentScene;

        std::function<void()> goToMenu;
        std::function<void()> startGame;
        std::function<void()> quitGame;

        quitGame = [&]() {
            window.close();
        };

        goToMenu = [&]() {
            if (currentScene) currentScene->onExit();
            currentScene = std::make_unique<tetris::MainMenuScene>(window, &settings, startGame, quitGame);
            currentScene->onEnter();
        };

        startGame = [&]() {
            if (currentScene) currentScene->onExit();
            currentScene = std::make_unique<tetris::TetrisScene>(window, &settings, goToMenu);
            currentScene->onEnter();
        };

        // Uygulamayı Ana Menü ile başlatıyoruz
        goToMenu();

        loop.run(
            [&](const sf::Event &event) {
                if (event.is<sf::Event::Closed>()) {
                    window.close();
                }

                if (const auto* res = event.getIf<sf::Event::Resized>()) {
                    updateViewport(window, tetris::WIN_W, tetris::WIN_H);
                }

                if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
                    if (key->code == sf::Keyboard::Key::F11) {
                        settings.fullscreen = !settings.fullscreen;
                        if (settings.fullscreen) {
                            window.create(sf::VideoMode::getDesktopMode(), "Tetris", sf::Style::Default, sf::State::Fullscreen);
                        } else {
                            window.create(sf::VideoMode({tetris::WIN_W, tetris::WIN_H}), "Tetris", sf::Style::Default, sf::State::Windowed);
                        }
                        window.setFramerateLimit(60);
                        updateViewport(window, tetris::WIN_W, tetris::WIN_H);

                        if (currentScene) currentScene->onEnter();
                    }
                }

                if (currentScene) {
                    currentScene->handleEvent(event);
                }
            },
            [&](float dt) {
                input.update();
                if (currentScene) {
                    currentScene->update(dt, input);
                }
            },
            [&](float alpha) {
                if (currentScene) {
                    currentScene->render(window, alpha);
                }
            }
        );

        if (currentScene) currentScene->onExit();

    } catch (const std::exception &e) {
        std::cerr << "[HATA] " << e.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}