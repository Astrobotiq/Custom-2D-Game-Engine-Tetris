//
// Created by e-r-e on 5/20/2026.
//

#ifndef SIMPLEENGINE2D_WINDOWUTILS_HPP
#define SIMPLEENGINE2D_WINDOWUTILS_HPP

#endif //SIMPLEENGINE2D_WINDOWUTILS_HPP
#pragma once
#include <SFML/Graphics.hpp>

inline void updateViewport(sf::RenderWindow& window, float targetWidth = 700.f, float targetHeight = 740.f) {
    sf::Vector2u size = window.getSize();
    float windowRatio = static_cast<float>(size.x) / static_cast<float>(size.y);
    float targetRatio = targetWidth / targetHeight;

    sf::View view(sf::FloatRect({0.f, 0.f}, {targetWidth, targetHeight}));

    float sizeX = 1.f;
    float sizeY = 1.f;
    float posX = 0.f;
    float posY = 0.f;

    if (windowRatio >= targetRatio) {
        sizeX = targetRatio / windowRatio;
        posX = (1.f - sizeX) / 2.f;
    } else {
        sizeY = windowRatio / targetRatio;
        posY = (1.f - sizeY) / 2.f;
    }

    view.setViewport(sf::FloatRect({posX, posY}, {sizeX, sizeY}));
    window.setView(view);
}