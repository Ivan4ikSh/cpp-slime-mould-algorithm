#include "domain.h"
#include "agent.h"

int main() {
    InitiliseConfig();
    sf::RenderWindow window(sf::VideoMode(config::WIDTH, config::HEIGHT), "Slime Mold");
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
    trail_map.create(config::WIDTH, config::HEIGHT);
    temp_tex_1.create(config::WIDTH, config::HEIGHT);
    temp_tex_2.create(config::WIDTH, config::HEIGHT);
    trail_map.clear(sf::Color::Black);

    srand(static_cast<unsigned>(time(nullptr)));
    sf::VertexArray agents_vertices(sf::Points, config::NUM_AGENTS);
    sf::RenderStates render_states;
    render_states.blendMode = sf::BlendAdd;

    std::vector<unsigned> rows(config::HEIGHT);
    std::iota(rows.begin(), rows.end(), 0);

    sf::Clock clock;
    float fps_alpha = 0.1f;
    float smoothed_fps = static_cast<float>(constant::FPS);
    sf::Clock fps_update_clock;
    std::vector<sf::Vector2f> food_positions;

    // В основном коде
    std::vector<Agent*> agents;

    // Сначала создаем всех агентов
    std::vector<Agent> agent_storage;
    agent_storage.reserve(config::NUM_AGENTS);
    for (size_t i = 0; i < config::NUM_AGENTS; ++i) {
        agent_storage.emplace_back(agents);
    }

    // Затем заполняем вектор указателей
    agents.reserve(config::NUM_AGENTS);
    for (auto& agent : agent_storage) {
        agents.push_back(&agent);
    }

    bool is_paused = true;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed || event.key.code == sf::Keyboard::Escape) window.close();
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::S) {
                    std::ofstream output_file("agents_data.csv");

                    if (output_file.is_open()) {
                        output_file << "X;Y;Weight\n";

                        for (const auto& agent : agents) {
                            float x = agent->GetPos().x;
                            float y = agent->GetPos().y;
                            float weight = agent->GetWeight();

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
                if (event.key.code == sf::Keyboard::Space) {
                    if (is_paused) is_paused = false;
                    else is_paused = true;
                }
            }
            if (event.type == sf::Event::MouseButtonPressed) {
                sf::Vector2f mouse_pos = window.mapPixelToCoords( { event.mouseButton.x, event.mouseButton.y } );

                if (event.mouseButton.button == sf::Mouse::Left) {
                    food_positions.push_back({ mouse_pos.x - food::RADIUS, mouse_pos.y - food::RADIUS });
                }

                if (event.mouseButton.button == sf::Mouse::Right && !food_positions.empty()) {
                    float min_dist = 25.0f; // Радиус удаления
                    auto closest = std::min_element(food_positions.begin(), food_positions.end(), [&](const sf::Vector2f& a, const sf::Vector2f& b) {
                            return Distance(mouse_pos, a) < Distance(mouse_pos, b);
                        });

                    if (Distance(mouse_pos, *closest) <= min_dist) {
                        food_positions.erase(closest);
                    }
                }
            }
        }
        if (!is_paused) {
            sf::Image trail_image = trail_map.getTexture().copyToImage();
            for (auto& agent : agents) {
                agent->Update(trail_image, food_positions);
            }
            if (!food_positions.empty()) population::LOCAL_TIME += 0.01f;
            else population::LOCAL_TIME = 0.0f;

            sf::Image img = trail_map.getTexture().copyToImage();
            std::for_each(std::execution::par, rows.begin(), rows.end(),
                [&](unsigned y) {
                    for (unsigned x = 0; x < config::WIDTH; ++x) {
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
            blur.first.setUniform("offset", simulation::BLUR_STRENGTH / config::WIDTH);
            temp_tex_1.clear(sf::Color::Transparent);
            temp_tex_1.draw(sf::Sprite(decayed_tex), &blur.first);
            temp_tex_1.display();

            blur.second.setUniform("texture", sf::Shader::CurrentTexture);
            blur.second.setUniform("offset", simulation::BLUR_STRENGTH / config::HEIGHT);
            temp_tex_2.clear(sf::Color::Transparent);
            temp_tex_2.draw(sf::Sprite(temp_tex_1.getTexture()), &blur.second);
            temp_tex_2.display();

            trail_map.clear();
            trail_map.draw(sf::Sprite(temp_tex_2.getTexture()));

            std::vector<size_t> indices(agents.size());
            std::iota(indices.begin(), indices.end(), 0);
            std::for_each(std::execution::par, indices.begin(), indices.end(), [&](size_t i) {
                agents_vertices[i].position = agents[i]->GetPos();
                agents_vertices[i].color = agent::COLOR;

                agents_vertices[i].color = sf::Color(
                    10 * agents[i]->GetWeight(),
                    10 * (1 - agents[i]->GetWeight()),
                    255
                );
                });

            trail_map.draw(agents_vertices, render_states);
            trail_map.display();
        }
        

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
        sf::CircleShape food_shape(food::RADIUS);
        food_shape.setFillColor(food::COLOR);
        for (const auto& pos : food_positions) {
            food_shape.setPosition(pos);
            window.draw(food_shape);
        }
        //for (const auto& button : buttons) {
        //    button.draw(window);
        //}
        sf::CircleShape best_pos_shape(food::RADIUS / 2.0f);
        best_pos_shape.setPosition({ population::BEST_POSITION.x + best_pos_shape.getRadius(), population::BEST_POSITION.y + best_pos_shape.getRadius() });
        best_pos_shape.setFillColor(sf::Color::Red);
        window.draw(best_pos_shape);
        window.display();
    }

    return 0;
}