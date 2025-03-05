#include "domain.h"
#include "random-hash.h"
#include "framework.h"

class Agent {
public:
    Agent() {
        auto [pos, heading] = InitiliseMode();
        position_ = pos;
        heading_ = heading;
        weight_ = 0.0f;

        PrecomputeSensorVectors();
        UpdateCosSin();
    }

    void Update(sf::Image& trail_map, const std::vector<sf::Vector2f>& food_positions) {
        // 2. Расчет адаптивного веса
        weight_ = CalculateCombinedWeight(position_, food_positions);

        // 3. Обновление колебательных параметров
        vb_ *= 0.995f;  // Постепенное уменьшение влияния
        vc_ = (rand() % 2000 - 1000) / 1000.0f;  // [-1, 1]

        // 4. Движение с учетом био-осцилляций
        sf::Vector2f new_position = position_;
        float oscillation = vb_ * vc_;

        new_position.x += cos_heading_ + oscillation * (rand() % 100 - 50) / 50.0f;
        new_position.y += sin_heading_ + oscillation * (rand() % 100 - 50) / 50.0f;

        uint32_t random = Hash(static_cast<uint32_t>(position_.y * Config::WIDTH + position_.x + Hash(this->heading_)));
        if (new_position.x < 0 || new_position.x >= Config::WIDTH || new_position.y < 0 || new_position.y >= Config::HEIGHT) {
            // Телепортация по оси X
            if (new_position.x < 0) new_position.x += Config::WIDTH;
            else if (new_position.x >= Config::WIDTH) new_position.x -= Config::WIDTH;

            // Телепортация по оси Y
            if (new_position.y < 0) new_position.y += Config::HEIGHT;
            else if (new_position.y >= Config::HEIGHT) new_position.y -= Config::HEIGHT;
        }
        // 6. Применение позиции
        position_ = new_position;

        const sf::Vector2f rotated_right{
             sensor_.right.x * cos_heading_ - sensor_.right.y * sin_heading_,
             sensor_.right.x * sin_heading_ + sensor_.right.y * cos_heading_
        };
        const sf::Vector2f rotated_left{
            sensor_.left.x * cos_heading_ - sensor_.left.y * sin_heading_,
            sensor_.left.x * sin_heading_ + sensor_.left.y * cos_heading_
        };
        const sf::Vector2f rotated_forward{
            sensor_.forward.x * cos_heading_ - sensor_.forward.y * sin_heading_,
            sensor_.forward.x * sin_heading_ + sensor_.forward.y * cos_heading_
        };

        const float r = GetSensorValue(trail_map, position_ + rotated_right);
        const float l = GetSensorValue(trail_map, position_ + rotated_left);
        const float f = GetSensorValue(trail_map, position_ + rotated_forward);

        // 12. Принятие решения о повороте
        float random_factor = (rand() % 100) / 100.0f * vb_;  // Случайность уменьшается со временем

        if (f > l && f > r) {
            // Движение прямо
        }
        else if (l > r + random_factor) {
            heading_ -= agent::ROTATION_ANGLE * (1.0f + vb_);
        }
        else if (r > l + random_factor) {
            heading_ += agent::ROTATION_ANGLE * (1.0f + vb_);
        }
        else {
            // Случайный выбор при равных показаниях
            heading_ += (rand() % 2 ? 1 : -1) * agent::ROTATION_ANGLE * vb_;
        }

        // 14. Обновление тригонометрических значений
        UpdateCosSin();
    }

    sf::Vector2f GetPos() const { return position_; }
    float GetWeight() const { return weight_; }
private:
    float heading_;
    float cos_heading_;
    float sin_heading_;
    sf::Vector2f position_;
    Sensor sensor_;
    float weight_;

    float vb_ = 1.0f;  // Начальное значение (уменьшается со временем)
    float vc_ = 0.5f;  // Случайное значение [-1, 1]

    void PrecomputeSensorVectors() {
        const float radian_angle = sensor::ANGLE * constant::PI / 180.0f;
        sensor_.right = {
            sensor::DISTANCE * cosf(radian_angle),
            sensor::DISTANCE * sinf(radian_angle)
        };
        sensor_.left = {
            sensor::DISTANCE * cosf(-radian_angle),
            sensor::DISTANCE * sinf(-radian_angle)
        };
        sensor_.forward = { sensor::DISTANCE, 0.0f };
    }

    void UpdateCosSin() {
        const float rad = heading_ * constant::PI / 180.0f;
        cos_heading_ = cosf(rad);
        sin_heading_ = sinf(rad);
    }

    float GetSensorValue(const sf::Image& trail_map, const sf::Vector2f& sensor_position) const {
        // Существующее значение следа
        const unsigned x = static_cast<unsigned>(std::clamp(sensor_position.x, 0.0f, static_cast<float>(Config::WIDTH - 1)));
        const unsigned y = static_cast<unsigned>(std::clamp(sensor_position.y, 0.0f, static_cast<float>(Config::HEIGHT - 1)));

        float sensor_value = (trail_map.getPixel(x, y).r + trail_map.getPixel(x, y).g + trail_map.getPixel(x, y).b) / 3.0f;
        return sensor_value * (1.0f + weight_);
    }
};

int main() {
    InitiliseConfig();
    sf::RenderWindow window(sf::VideoMode(Config::WIDTH, Config::HEIGHT), "Slime Mold");
    window.setFramerateLimit(constant::FPS);

    sf::Font font;
    if (!font.loadFromFile("C:/Users/user/source/repos/test/arial.ttf")) {
        std::cerr << "Error loading font\n";
        return EXIT_FAILURE;
    }

    std::pair<sf::Shader, sf::Shader> blur;
    if (!blur.first.loadFromMemory(shader::HORIZONTAL_BLUR, sf::Shader::Fragment) ||
        !blur.second.loadFromMemory(shader::VERTICAL_BLUR, sf::Shader::Fragment)) {
        return EXIT_FAILURE;
    }

    std::vector<Button> buttons;
    CreateButtons(buttons, font);

    sf::Text fps_text;
    fps_text.setFont(font);
    fps_text.setCharacterSize(20);
    fps_text.setPosition(10, 10);

    sf::RenderTexture trail_map, temp_tex_1, temp_tex_2;
    trail_map.create(Config::WIDTH, Config::HEIGHT);
    temp_tex_1.create(Config::WIDTH, Config::HEIGHT);
    temp_tex_2.create(Config::WIDTH, Config::HEIGHT);
    trail_map.clear(sf::Color::Black);

    srand(static_cast<unsigned>(time(nullptr)));
    std::vector<Agent> agents(Config::NUM_AGENTS);
    sf::VertexArray agents_vertices(sf::Points, Config::NUM_AGENTS);
    sf::RenderStates render_states;
    render_states.blendMode = sf::BlendAdd;

    std::vector<unsigned> rows(Config::HEIGHT);
    std::iota(rows.begin(), rows.end(), 0);

    sf::Clock clock;
    float fps_alpha = 0.1f;
    float smoothed_fps = static_cast<float>(constant::FPS);
    sf::Clock fps_update_clock;
    std::vector<sf::Vector2f> food_positions;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::KeyPressed) {
                // Проверяем нажатие именно пробела
                if (event.key.code == sf::Keyboard::Space) {
                    // Открываем файл для записи (перезаписываем предыдущие данные)
                    std::ofstream output_file("agents_data.csv");

                    if (output_file.is_open()) {
                        // Записываем заголовки CSV
                        output_file << "X;Y;Weight\n";

                        // Проходим по всем агентам
                        for (const auto& agent : agents) { // Предполагается, что agents - ваш контейнер с агентами
                            // Получаем данные агента
                            float x = agent.GetPos().x;
                            float y = agent.GetPos().y;
                            float weight = agent.GetWeight(); // Предполагается, что вес хранится в поле weight

                            // Записываем в файл через разделитель ;
                            output_file.precision(16);
                            output_file << x << ";" << y << ";" << weight << "\n";
                        }

                        output_file.close();
                        std::cout << "Данные успешно сохранены в agents_data.csv\n";
                    }
                    else {
                        std::cerr << "Ошибка открытия файла!\n";
                    }
                }
            }
            if (event.type == sf::Event::Closed) window.close();
            if (event.type == sf::Event::MouseButtonPressed) {
                sf::Vector2f mouse_pos = window.mapPixelToCoords( { event.mouseButton.x, event.mouseButton.y } );

                // Левая кнопка - добавить еду
                if (event.mouseButton.button == sf::Mouse::Left) {
                    food_positions.push_back({ mouse_pos.x - food::RADIUS, mouse_pos.y - food::RADIUS });
                }

                // Правая кнопка - удалить ближайшую еду
                if (event.mouseButton.button == sf::Mouse::Right && !food_positions.empty()) {
                    float min_dist = 25.0f; // Радиус удаления
                    auto closest = std::min_element(food_positions.begin(), food_positions.end(), [&](const sf::Vector2f& a, const sf::Vector2f& b) {
                            return distance(mouse_pos, a) < distance(mouse_pos, b);
                        });

                    if (distance(mouse_pos, *closest) <= min_dist) {
                        food_positions.erase(closest);
                    }
                }
            }

            //for (auto& button : buttons) {
            //    button.handleEvent(event, window);
            //}
        }
        sf::Image trail_image = trail_map.getTexture().copyToImage();
        for (auto& agent : agents) {
            agent.Update(trail_image, food_positions);
        }
        //std::for_each(std::execution::par, agents.begin(), agents.end(),
        //    [&](Agent& agent) { agent.Update(trail_image, food_positions); });

        sf::Image img = trail_map.getTexture().copyToImage();
        std::for_each(std::execution::par, rows.begin(), rows.end(),
            [&](unsigned y) {
                for (unsigned x = 0; x < Config::WIDTH; ++x) {
                    auto pixel = img.getPixel(x, y);
                    pixel.r = static_cast<sf::Uint8>(pixel.r * simulation::DECAY_RATE);
                    pixel.g = static_cast<sf::Uint8>(pixel.g * simulation::DECAY_RATE);
                    pixel.b = static_cast<sf::Uint8>(pixel.b * simulation::DECAY_RATE);
                    img.setPixel(x, y, pixel);
                }
            });

        sf::Texture decayed_tex;
        decayed_tex.loadFromImage(img);

        blur.first.setUniform("texture", sf::Shader::CurrentTexture);
        blur.first.setUniform("offset", simulation::BLUR_STRENGTH / Config::WIDTH);
        temp_tex_1.clear(sf::Color::Transparent);
        temp_tex_1.draw(sf::Sprite(decayed_tex), &blur.first);
        temp_tex_1.display();

        blur.second.setUniform("texture", sf::Shader::CurrentTexture);
        blur.second.setUniform("offset", simulation::BLUR_STRENGTH / Config::HEIGHT);
        temp_tex_2.clear(sf::Color::Transparent);
        temp_tex_2.draw(sf::Sprite(temp_tex_1.getTexture()), &blur.second);
        temp_tex_2.display();

        trail_map.clear();
        trail_map.draw(sf::Sprite(temp_tex_2.getTexture()));

        std::vector<size_t> indices(agents.size());
        std::iota(indices.begin(), indices.end(), 0);
        std::for_each(std::execution::par, indices.begin(), indices.end(), [&](size_t i) {
            agents_vertices[i].position = agents[i].GetPos();
            agents_vertices[i].color = agent::COLOR;

            agents_vertices[i].color = sf::Color(
                10 * agents[i].GetWeight(),
                10 * (1 - agents[i].GetWeight()),
                255
            );
            });

        trail_map.draw(agents_vertices, render_states);
        trail_map.display();

        float delta_time = clock.restart().asSeconds();
        if (delta_time > 0) {
            float current_fps = 1.0f / delta_time;
            smoothed_fps = smoothed_fps * (1.0f - fps_alpha) + current_fps * fps_alpha;
        }

        if (fps_update_clock.getElapsedTime().asMilliseconds() > 100) {
            if (smoothed_fps >= 20) fps_text.setFillColor(sf::Color::Green);
            else fps_text.setFillColor(sf::Color::Red);
            fps_text.setString("FPS: " + std::to_string(static_cast<int>(smoothed_fps)));
            fps_update_clock.restart();
        }

        window.clear();
        window.draw(sf::Sprite(trail_map.getTexture()));
        window.draw(fps_text);
        sf::CircleShape food_shape(food::RADIUS); // Размер точки
        food_shape.setFillColor(food::COLOR);
        for (const auto& pos : food_positions) {
            food_shape.setPosition(pos);
            window.draw(food_shape);
        }
        //for (const auto& button : buttons) {
        //    button.draw(window);
        //}
        window.display();
    }

    return 0;
}