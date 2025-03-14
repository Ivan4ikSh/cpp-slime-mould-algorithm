#pragma once
#include "domain.h"

void CreateMaze(sf::RenderTexture& walls_texture) {
    walls_texture.clear(sf::Color::Transparent);

    //sf::Color wall_color = sf::Color(255, 255, 255, 80);
    sf::Color wall_color = sf::Color::White;

    for (int y = 0; y < maze::MAZE.size(); ++y) {
        for (int x = 0; x < maze::MAZE[0].size(); ++x) {
            if (maze::MAZE[y][x] == 1) {
                sf::RectangleShape wall(sf::Vector2f(maze::CELL_SIZE, maze::CELL_SIZE));
                wall.setPosition(x * maze::CELL_SIZE, y * maze::CELL_SIZE);
                wall.setFillColor(wall_color);
                walls_texture.draw(wall);
            }
        }
    }

    walls_texture.display();
}

float CalculateA() {
    return std::atanh(-(simulation::ITER / static_cast<float>(simulation::MAX_ITERATION)) + 1);
}

float CalculateVB() {
    float a = CalculateA();
    return (rand() % 2000 - 1000) / 1000.0f * CalculateA() * simulation::A_DIFFUSION_STRENGTH;
}

float CalculateVC() {
    return 1.0f - simulation::ITER / static_cast<float>(simulation::MAX_ITERATION);
}

float Distance(const sf::Vector2f& a, const sf::Vector2f& b) {
    return static_cast<float>(std::sqrt(std::pow(a.x - b.x, 2) + std::pow(a.y - b.y, 2)));
}

float FitnessFunc(const sf::Vector2f& a, const sf::Vector2f& b) {
    return Distance(a, b);
}

bool IsTopHalf(float current_fitness) {
    return current_fitness > std::abs(population::BEST_FITNESS - population::WORST_FITNESS) / 2.0f;
}

float CalculateAgentWeight(const sf::Vector2f& agent_pos, const sf::Vector2f& food_pos) {
    float fitness = FitnessFunc(agent_pos, food_pos);

    population::BEST_FITNESS = std::min(population::BEST_FITNESS, fitness);
    population::WORST_FITNESS = std::max(population::WORST_FITNESS, fitness);

    if (population::WORST_FITNESS == population::BEST_FITNESS) return 1.0f;

    fitness = (fitness - population::BEST_FITNESS) / (population::WORST_FITNESS - population::BEST_FITNESS);

    float r = ScaleToRange01(Hash(rand()));
    float weight;
    if (IsTopHalf(fitness)) return 1.0f + r * std::log(fitness + 1.0f);
    else return 1.0f - r * std::log(fitness + 1.0f);
}

float CalculateCombinedWeight(const sf::Vector2f& agent_pos,
    const std::vector<sf::Vector2f>& food_sources) {
    if (food_sources.empty()) return 0.0f;

    float total_weight = 0.0f;
    float sum_influence = 0.0f;

    for (const auto& food : food_sources) {
        float w = CalculateAgentWeight(agent_pos, food);
        float influence = FitnessFunc(agent_pos, food);
        total_weight += w * influence;
        sum_influence += influence;
    }

    return (sum_influence > 0) ? total_weight / sum_influence : 0.0f;
}

void InitiliseConfig() {
    switch (frame::CURRENT) {
    case frame::MINI:   config::WIDTH = 320;   config::HEIGHT = 180;   config::NUM_AGENTS = 5'000;
        break;
    case frame::SMALL:  config::WIDTH = 640;   config::HEIGHT = 480;   config::NUM_AGENTS = 20'000;
        break;
    case frame::MEDIUM: config::WIDTH = 1280;  config::HEIGHT = 720;   config::NUM_AGENTS = 100'000;
        break;
    case frame::BIG:    config::WIDTH = 1920;  config::HEIGHT = 1080;  config::NUM_AGENTS = 1'000'000;
        break;
    }
}

std::pair<sf::Vector2f, float> InitiliseMode() {
    float heading;
    sf::Vector2f position;

    switch (mode::CURRENT) {
    case mode::NOISE: {
        position = { static_cast<float>(Hash(rand()) % config::WIDTH), static_cast<float>(Hash(rand()) % config::HEIGHT) };
        heading = static_cast<float>(Hash(rand()) % 360);
        break;
    }
    case mode::CIRCLE: {
        const float center_x = config::WIDTH / 2.0f;
        const float center_y = config::HEIGHT / 2.0f;

        const float max_radius = std::min(config::WIDTH, config::HEIGHT) / 2.0f * 0.8f;
        float random_factor = static_cast<float>(rand()) / RAND_MAX;
        float r = sqrtf(random_factor) * max_radius;
        float theta = static_cast<float>(rand()) / RAND_MAX * 2.0f * constant::PI;

        position.x = center_x + r * cosf(theta);
        position.y = center_y + r * sinf(theta);

        float dx = center_x - position.x;
        float dy = center_y - position.y;
        float angle_rad = atan2f(dy, dx);

        heading = angle_rad * 180.0f / constant::PI;
        if (heading < 0.0f) heading += 360.0f;
        break;
    }
    case mode::CENTER: {
        position = {
            static_cast<float>(config::WIDTH / 2),
            static_cast<float>(config::HEIGHT / 2)
        };
        heading = static_cast<float>(rand() % 360);
        break;
    }
    case mode::TWO_POINTS: {
        heading = static_cast<float>(rand() % 360);
        uint32_t random = Hash(static_cast<uint32_t>(Hash(heading)));

        if (ScaleToRange01(random) >= 0.5) {
            position = { static_cast<float>(config::WIDTH / 3),  static_cast<float>(config::HEIGHT / 2) };
        }
        else {
            position = { static_cast<float>(2 * config::WIDTH / 3),  static_cast<float>(config::HEIGHT / 2) };
        }
        break;
    }
    case mode::THREE_POINTS: {
        heading = static_cast<float>(rand() % 360);
        uint32_t random = Hash(static_cast<uint32_t>(Hash(heading)));

        if (ScaleToRange01(random) <= 0.3) {
            position = { static_cast<float>(config::WIDTH / 2),  static_cast<float>(2 * config::HEIGHT / 3) };
        }
        else if (ScaleToRange01(random) <= 0.6) {
            position = { static_cast<float>(config::WIDTH / 3),  static_cast<float>(config::HEIGHT / 3) };
        }
        else {
            position = { static_cast<float>(2 * config::WIDTH / 3),  static_cast<float>(config::HEIGHT / 3) };
        }
        break;
    }
    default:
        break;
    }
    //if (mode::IS_MAZE) {
    //    position = {
    //            static_cast<float>(maze::WALL_THICKNESS + 10.0f),
    //            static_cast<float>(config::HEIGHT - maze::WALL_THICKNESS - 10.0f)
    //    };
    //    heading = static_cast<float>(rand() % 360);
    //}

    return { position, heading };
}