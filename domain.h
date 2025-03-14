#pragma once
#include <cmath>
#include <vector>
#include <fstream>
#include <algorithm>
#include <execution>
#include <ctime>
#include <numeric>
#include <random>
#include <iostream>
#include <functional>

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include "random-hash.h"

struct Sensor {
    sf::Vector2f right;
    sf::Vector2f left;
    sf::Vector2f forward;
};

namespace config {
    unsigned WIDTH;
    unsigned HEIGHT;
    unsigned NUM_AGENTS;
};

namespace constant {
    double ALPHA = 0.001;
    int TIME = 0;

    const float PI = 3.1415926535897932384626433832795028f;
    const int FPS = 40;
}

namespace food {
    const float RADIUS = 3.0f;
    sf::Color COLOR = sf::Color::Green;
}

namespace mode {
    bool IS_POLLING = true;
    bool IS_MAZE = false;
    bool IS_RUN = true;

    enum Type {
        NOISE,
        CIRCLE,
        CENTER,
        TWO_POINTS,
        THREE_POINTS
    };
    Type CURRENT = CIRCLE;
}

namespace frame {
    enum Size {
        MINI,
        SMALL,
        MEDIUM,
        BIG
    };
    Size CURRENT = MINI;
}

namespace sensor {
    const float BOOST = 5.0f;
    const float DISTANCE = 15.0f;
    const float ANGLE = 22.5f;
}

namespace agent {
    float SPEED = 0.8f;
    float ROTATION_ANGLE = 22.5f;
    sf::Color COLOR = sf::Color(255, 255, 255, 70);
}

namespace population {
    sf::Vector2f BEST_POSITION;
    float BEST_FITNESS;
    float WORST_FITNESS;
};

namespace simulation {
    int ITER = 1;
    int MAX_ITERATION = 10;
    sf::Vector2f BEST_FOOD_POS = { 0.0f,0.0f };
    const float DECAY_RATE = 0.97f;
    const float BOUNDARY_OFFSET = 0.0f;
    const float BLUR_STRENGTH = 0.2f;
    const float A_DIFFUSION_STRENGTH = 1.0f;
    const float ANGLE_RESPONSIVNESS = 0.1f;

}

namespace shader {
    const std::string HORIZONTAL_BLUR = R"(
        uniform sampler2D texture;
        uniform float offset;
    
        void main() {
            vec2 texCoords = gl_TexCoord[0].xy;
            vec4 color = texture2D(texture, texCoords) * 0.227027;
            color += texture2D(texture, texCoords + vec2(offset * 1.0, 0.0)) * 0.1945946;
            color += texture2D(texture, texCoords - vec2(offset * 1.0, 0.0)) * 0.1945946;
            color += texture2D(texture, texCoords + vec2(offset * 2.0, 0.0)) * 0.1216216;
            color += texture2D(texture, texCoords - vec2(offset * 2.0, 0.0)) * 0.1216216;
            color += texture2D(texture, texCoords + vec2(offset * 3.0, 0.0)) * 0.054054;
            color += texture2D(texture, texCoords - vec2(offset * 3.0, 0.0)) * 0.054054;
            color += texture2D(texture, texCoords + vec2(offset * 4.0, 0.0)) * 0.016216;
            color += texture2D(texture, texCoords - vec2(offset * 4.0, 0.0)) * 0.016216;
            gl_FragColor = color;
        }
    )";
    const std::string VERTICAL_BLUR = R"(
        uniform sampler2D texture;
        uniform float offset;
    
        void main() {
            vec2 texCoords = gl_TexCoord[0].xy;
            vec4 color = texture2D(texture, texCoords) * 0.227027;
            color += texture2D(texture, texCoords + vec2(0.0, offset * 1.0)) * 0.1945946;
            color += texture2D(texture, texCoords - vec2(0.0, offset * 1.0)) * 0.1945946;
            color += texture2D(texture, texCoords + vec2(0.0, offset * 2.0)) * 0.1216216;
            color += texture2D(texture, texCoords - vec2(0.0, offset * 2.0)) * 0.1216216;
            color += texture2D(texture, texCoords + vec2(0.0, offset * 3.0)) * 0.054054;
            color += texture2D(texture, texCoords - vec2(0.0, offset * 3.0)) * 0.054054;
            color += texture2D(texture, texCoords + vec2(0.0, offset * 4.0)) * 0.016216;
            color += texture2D(texture, texCoords - vec2(0.0, offset * 4.0)) * 0.016216;
            gl_FragColor = color;
        }
    )";
}

namespace maze {
    const float WALL_THICKNESS = 5.0f;
    const float CELL_SIZE = 10.5f;

    const std::vector<std::vector<int>> nagaki = {
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1},
        {1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1},
        {1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1},
        {1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1},
        {1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1},
        {1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1},
        {1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1},
        {1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1},
        {1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1},
        {1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1},
        {1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1},
        {1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1},
        {1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1},
        {1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
    };
    const std::vector<std::vector<int>> test1 = {
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
    };

    const std::vector<std::vector<int>> test2 = {
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
    };

    std::vector<std::vector<int>> MAZE = test2;
}