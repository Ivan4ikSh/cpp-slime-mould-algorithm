#pragma once
#include "domain.h"

struct Button {
    Button(const sf::Vector2f& position, const std::string& label, const sf::Font& font, std::function<void()> onClick) : action(onClick) {
        shape.setSize(size);
        shape.setPosition(position);
        shape.setFillColor(sf::Color::White);
        shape.setOutlineThickness(2);
        shape.setOutlineColor(sf::Color::Black);

        text.setFont(font);
        text.setString(label);
        text.setCharacterSize(16);
        text.setFillColor(sf::Color::Black);
        sf::FloatRect textBounds = text.getLocalBounds();
        text.setOrigin(textBounds.width / 2, textBounds.height / 2);
        text.setPosition(position.x + size.x / 2, position.y + size.y / 2);
    }

    bool isMouseOver(const sf::RenderWindow& window) const {
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        return shape.getGlobalBounds().contains(static_cast<sf::Vector2f>(mousePos));
    }

    void draw(sf::RenderWindow& window) const {
        window.draw(shape);
        window.draw(text);
    }

    void handleEvent(const sf::Event& event, const sf::RenderWindow& window) {
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left &&
            isMouseOver(window)) {
            action();
        }
    }

    sf::Vector2f size = { 80, 30 };
    sf::RectangleShape shape;
    sf::Text text;
    std::function<void()> action;
};

// Функция для вычисления гиперболического тангенса
double tanh(double x) {
    return (exp(x) - exp(-x)) / (exp(x) + exp(-x));
}

// Функция для вычисления arctanh
double arctanh(double x) {
    return 0.5 * log((1 + x) / (1 - x));
}

float distance(const sf::Vector2f& a, const sf::Vector2f& b) {
    return static_cast<float>(std::sqrt(std::pow(a.x - b.x, 2) + std::pow(a.y - b.y, 2)));
}

bool IsTopHalf(float current_fitness) {
    return current_fitness > std::abs(population::BEST_FITNES - population::WORST_FITNES) / 2.0f;
}

float CalculateAgentWeight(const sf::Vector2f& agent_pos, const sf::Vector2f& food_pos) {
    // 1. Рассчитываем расстояние от агента до еды
    float fitness = distance(agent_pos, food_pos); // Фитнес = расстояние

    agent::BEST_FITNESS = std::min(agent::BEST_FITNESS, fitness);
    agent::WORST_FITNESS = std::max(agent::WORST_FITNESS, fitness);
    // 3. Нормализуем расстояние в диапазон [0, 1]
    if (agent::WORST_FITNESS == agent::BEST_FITNESS) return 1.0f;

    fitness = (fitness - agent::BEST_FITNESS) / (agent::WORST_FITNESS - agent::BEST_FITNESS);
    //normalized = std::max(0.0f, std::min(1.0f, normalized)); // Ограничиваем

    // 4. Формула веса (адаптированная из SMA)
    float r = 0.5f;
    float weight;
    if (IsTopHalf(fitness)) {
        // Первые 50% агентов (ближе к еде)
        weight = 1.0f + r * std::log(fitness + 1.0f);
    }
    else {
        // Остальные 50% (дальше от еды)
        weight = 1.0f - r * std::log(fitness + 1.0f);
    }
    return weight;
}

float CalculateCombinedWeight(const sf::Vector2f& agent_pos,
    const std::vector<sf::Vector2f>& food_sources) {
    if (food_sources.empty()) return 0.0f;

    float total_weight = 0.0f;
    float sum_influence = 0.0f;

    for (const auto& food : food_sources) {
        float w = CalculateAgentWeight(agent_pos, food);
        float influence = 1.0f / distance(agent_pos, food); // Чем ближе, тем больше влияние
        total_weight += w * influence;
        sum_influence += influence;
    }

    return (sum_influence > 0) ? total_weight / sum_influence : 0.0f;
}

void InitiliseConfig() {
    switch (frame::CURRENT) {
    case frame::MINI:  Config::WIDTH = 500;   Config::HEIGHT = 500;   Config::NUM_AGENTS = 5'000;    break;
    case frame::SMALL:  Config::WIDTH = 640;   Config::HEIGHT = 480;   Config::NUM_AGENTS = 25'000;    break;
    case frame::MEDIUM: Config::WIDTH = 1280;  Config::HEIGHT = 720;   Config::NUM_AGENTS = 100'000;    break;
    case frame::BIG:    Config::WIDTH = 1920;  Config::HEIGHT = 1080;  Config::NUM_AGENTS = 1'000'000;   break;
    }
}

std::pair<sf::Vector2f, float> InitiliseMode() {
    float heading;
    sf::Vector2f position;

    switch (mode::CURRENT) {
    case mode::NOISE: {
        position = { static_cast<float>(Hash(rand()) % Config::WIDTH), static_cast<float>(Hash(rand()) % Config::HEIGHT) };
        heading = static_cast<float>(Hash(rand()) % 360);
        break;
    }
    case mode::CIRCLE: {
        const float center_x = Config::WIDTH / 2.0f;
        const float center_y = Config::HEIGHT / 2.0f;

        const float max_radius = std::min(Config::WIDTH, Config::HEIGHT) / 2.0f * 0.8f;
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
            static_cast<float>(Config::WIDTH / 2),
            static_cast<float>(Config::HEIGHT / 2)
        };
        heading = static_cast<float>(rand() % 360);
        break;
    }
    case mode::TWO_POINTS: {
        heading = static_cast<float>(rand() % 360);
        uint32_t random = Hash(static_cast<uint32_t>(Hash(heading)));

        if (ScaleToRange01(random) >= 0.5) {
            position = { static_cast<float>(Config::WIDTH / 3),  static_cast<float>(Config::HEIGHT / 2) };
        }
        else {
            position = { static_cast<float>(2*Config::WIDTH / 3),  static_cast<float>(Config::HEIGHT / 2) };
        }
        break;
    }
    case mode::THREE_POINTS: {
        heading = static_cast<float>(rand() % 360);
        uint32_t random = Hash(static_cast<uint32_t>(Hash(heading)));

        if (ScaleToRange01(random) <= 0.3) {
            position = { static_cast<float>(Config::WIDTH / 2),  static_cast<float>(2 * Config::HEIGHT / 3) };
        }
        else if (ScaleToRange01(random) <= 0.6) {
            position = { static_cast<float>(Config::WIDTH / 3),  static_cast<float>(Config::HEIGHT / 3) };
        }
        else {
            position = { static_cast<float>(2 * Config::WIDTH / 3),  static_cast<float>(Config::HEIGHT / 3) };
        }
        break;
    }
    default:
        break;
    }
    return { position, heading };
}

void CreateButtons(std::vector<Button>& buttons, const sf::Font& font) {
    buttons.emplace_back(
        sf::Vector2f(Config::WIDTH - 200, 10), "+Speed", font, []() {
            agent::SPEED += 0.5f;
        });

    buttons.emplace_back(
        sf::Vector2f(Config::WIDTH - 100, 10), "-Speed", font, []() {
            agent::SPEED = std::max(0.1f, agent::SPEED - 0.5f);
        });
    buttons.emplace_back(
        sf::Vector2f(Config::WIDTH - 200, 50), "Pause", font, []() {
            agent::SPEED = 0;
        });
    buttons.emplace_back(
        sf::Vector2f(Config::WIDTH - 100, 50), "Resume", font, []() {
            agent::SPEED += 1.0f;
        });
}

/*
float CalculateAgentWeight(const sf::Vector2f& agent_pos,
    const sf::Vector2f& food_pos) {
    float dist = distance(agent_pos, food_pos);

    // Динамическое обновление лучшего/худшего расстояния
    static float global_min = INFINITY;
    static float global_max = -INFINITY;

    global_min = std::min(global_min, dist);
    global_max = std::max(global_max, dist);

    // Нормализация расстояния
    float normalized = (dist - global_min) / (global_max - global_min + 1e-5f);

    // Формула, стимулирующая нахождение "между" источниками
    float r = 0.5f;
    return 1.0f - r * std::log(normalized + 1.0f);
}
*/