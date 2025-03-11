#pragma once
#include "domain.h"
#include "framework.h"

class Agent {
public:
    Agent(const std::vector<Agent*>& agents) : population_(agents) {
        auto [pos, heading] = InitiliseMode();
        position_ = pos;
        heading_ = heading;
        weight_ = 0.0f;
        PrecomputeSensorVectors();
        UpdateCosSin();
    }

    void Update(sf::Image& trail_map, const std::vector<sf::Vector2f>& food_positions) {
        weight_ = 0.0f;
        if (!food_positions.empty()) {
            weight_ = CalculateCombinedWeight(position_, food_positions);
            float fitness = std::numeric_limits<float>::max();
            for (const auto& food : food_positions) {
                float fitness_func = FitnessFunc(position_, food);
                if (fitness_func < fitness) fitness = fitness_func;
            }
            population::BEST_FITNESS = std::min(population::BEST_FITNESS, fitness);


            const float p = tanh(std::abs(population::BEST_FITNESS - fitness));
            const float random = ScaleToRange01(Hash(rand()));

            const sf::Vector2f XA = GetRandomAgentPosition();
            const sf::Vector2f XB = GetRandomAgentPosition();
            const float vb = CalculateVB();
            const float vc = CalculateVC();
            population::BEST_POSITION = FindGlobalBestFood(food_positions);

            if (random < p) choosen_food_position_ = population::BEST_POSITION + vb * (weight_ * XA - XB);
            else choosen_food_position_ = vc * position_;

            if (Distance(choosen_food_position_, position_) < 5.0f) DepositPheromone(trail_map);

        }

        Exploration(trail_map, food_positions);
        UpdateCosSin();

        if (heading_ < 0) heading_ += 360;
        else if (heading_ >= 360) heading_ -= 360;
    }

    sf::Vector2f GetPos() const { return position_; }
    float GetWeight() const { return weight_; }
private:
    const std::vector<Agent*>& population_;

    float heading_;
    float cos_heading_;
    float sin_heading_;
    sf::Vector2f position_;
    sf::Vector2f choosen_food_position_;
    Sensor sensor_;
    float weight_;

    void Exploration(sf::Image& trail_map, const std::vector<sf::Vector2f>& food_positions) {
        sf::Vector2f new_position = position_;
        new_position.x += cos_heading_ * agent::SPEED;
        new_position.y += sin_heading_ * agent::SPEED;

        if (mode::IS_MAZE && IsWallCollision(new_position)) {
            int cell_x = static_cast<int>(position_.x / maze::CELL_SIZE);
            int cell_y = static_cast<int>(position_.y / maze::CELL_SIZE);

            float cell_left = cell_x * maze::CELL_SIZE;
            float cell_right = cell_left + maze::CELL_SIZE;
            float cell_top = cell_y * maze::CELL_SIZE;
            float cell_bottom = cell_top + maze::CELL_SIZE;

            bool hit_left = (new_position.x < cell_left && position_.x >= cell_left);
            bool hit_right = (new_position.x > cell_right && position_.x <= cell_right);
            bool hit_top = (new_position.y < cell_top && position_.y >= cell_top);
            bool hit_bottom = (new_position.y > cell_bottom && position_.y <= cell_bottom);

            float random_angle = ((rand() % 41) - 20);

            if (hit_left) new_position.x += maze::CELL_SIZE / 10 + 1;
            if (hit_right) new_position.x -= maze::CELL_SIZE / 10 + 1;
            if (hit_top) new_position.y += maze::CELL_SIZE / 10 + 1;
            if (hit_bottom) new_position.y -= maze::CELL_SIZE / 10 + 1;

            if (hit_left || hit_right) {
                heading_ = 180 - heading_ + random_angle;
            }
            else if (hit_top || hit_bottom) {
                heading_ = 360 - heading_ + random_angle;
            }

            if (heading_ < 0) heading_ += 360;
            if (heading_ >= 360) heading_ -= 360;
            weight_ = 0.0f;
        }

        if (IsBodrer(new_position)) {
            if (mode::IS_POLLING) {
                float random_angle = ((rand() % 41) - 20);

                if (new_position.x < 0 || new_position.x >= config::WIDTH) heading_ = 180 - heading_ + random_angle;
                else heading_ = 360 - heading_ + random_angle;

                if (heading_ < 0) heading_ += 360;
                if (heading_ >= 360) heading_ -= 360;

                if (new_position.x < 0) new_position.x = 1;
                else if (new_position.x >= config::WIDTH) new_position.x = config::WIDTH - 2;

                if (new_position.y < 0) new_position.y = 1;
                else if (new_position.y >= config::HEIGHT) new_position.y = config::HEIGHT - 2;
            }
            else {
                if (new_position.x < 0) new_position.x += config::WIDTH;
                else if (new_position.x >= config::WIDTH) new_position.x -= config::WIDTH;

                if (new_position.y < 0) new_position.y += config::HEIGHT;
                else if (new_position.y >= config::HEIGHT) new_position.y -= config::HEIGHT;
            }
            weight_ = 0.0f;
        }
        if (mode::IS_RUN) position_ = new_position;

        FollowPheromoneGradient(trail_map, food_positions);
    }
    
    bool IsWallCollision(const sf::Vector2f& position) const {
        int cell_x = static_cast<int>(position.x / maze::CELL_SIZE);
        int cell_y = static_cast<int>(position.y / maze::CELL_SIZE);

        if (cell_x <= 0 || cell_x >= maze::MAZE[0].size() || cell_y <= 0 || cell_y >= maze::MAZE.size()) return true;

        return maze::MAZE[cell_y][cell_x] == 1;
    }

    void FollowPheromoneGradient(sf::Image& trail_map, const std::vector<sf::Vector2f>& food_positions) {
        // Считываем значения сенсоров
        const sf::Vector2f right_sensor_pos = position_ + RotateVector(sensor_.right);
        const sf::Vector2f left_sensor_pos = position_ + RotateVector(sensor_.left);
        const sf::Vector2f forward_sensor_pos = position_ + RotateVector(sensor_.forward);

        float r = GetSensorValue(trail_map, right_sensor_pos);
        float l = GetSensorValue(trail_map, left_sensor_pos);
        float f = GetSensorValue(trail_map, forward_sensor_pos);

        float angle_boost = 1;
        float dynamic_rotation_angle = 0;
        if (!food_positions.empty()) {
            const float sensor_boost = (1 + weight_);
            const float right_sensor_dst = Distance(choosen_food_position_, right_sensor_pos);
            const float left_sensor_dst = Distance(choosen_food_position_, left_sensor_pos);
            const float forward_sensor_dst = Distance(choosen_food_position_, forward_sensor_pos);

            if (right_sensor_dst < left_sensor_dst && right_sensor_dst < forward_sensor_dst) r += sensor_boost * sensor::BOOST;
            else if (left_sensor_dst < right_sensor_dst && left_sensor_dst < forward_sensor_dst) l += sensor_boost * sensor::BOOST;
            else if (forward_sensor_dst < left_sensor_dst && forward_sensor_dst < right_sensor_dst) f += sensor_boost * sensor::BOOST;

            angle_boost = (sensor_boost * (1 + weight_));
            dynamic_rotation_angle = CalculateDynamicRotationAngle(Distance(position_, choosen_food_position_)) / angle_boost;
        }

        float max_val = std::max({ r, l, f });
        float sum = r + l + f;

        if (sum == 0) return;

        // Вероятностный выбор направления
        float rand_val = static_cast<float>(rand()) / RAND_MAX;
        if (rand_val < (r / sum)) heading_ += agent::ROTATION_ANGLE + dynamic_rotation_angle;
        else if (rand_val < ((r + l) / sum)) heading_ -= agent::ROTATION_ANGLE + dynamic_rotation_angle;
    }

    void DepositPheromone(sf::Image& trail_map) {
        // Откладываем феромон при движении
        sf::Color color = trail_map.getPixel(position_.x, position_.y);
        color.r = std::min(255, color.r + 50);
        trail_map.setPixel(position_.x, position_.y, color);
    }

    sf::Vector2f GetRandomAgentPosition() {
        return population_[rand() % population_.size()]->position_;
    }

    sf::Vector2f RotateVector(const sf::Vector2f& vec) {
        return {
            vec.x * cos_heading_ - vec.y * sin_heading_,
            vec.x * sin_heading_ + vec.y * cos_heading_
        };
    }

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

    float GetSensorValue(const sf::Image& trail_map, const sf::Vector2f& sensor_position) {
        const unsigned x = static_cast<unsigned>(std::clamp(sensor_position.x, 0.0f, static_cast<float>(config::WIDTH - 1)));
        const unsigned y = static_cast<unsigned>(std::clamp(sensor_position.y, 0.0f, static_cast<float>(config::HEIGHT - 1)));

        sf::Color color = trail_map.getPixel(x, y);
        return (color.r + color.g + color.b) / 3.0f;
    }

    float CalculateDirection(sf::Vector2f target_pos) {
        const float delta_x = target_pos.x - position_.x;
        const float delta_y = target_pos.y - position_.y;
        const float angle_rad = std::atan2(delta_y, delta_x);

        float angle_deg = angle_rad * 180.0f / static_cast<float>(constant::PI);
        return std::fmod(angle_deg + 360.0f, 360.0f);
    }

    float CalculateDynamicRotationAngle(float distance_to_food) {
        const float responsiveness = 0.9f;
        return agent::ROTATION_ANGLE * (1.0f + responsiveness * (1.0f - weight_));
    }

    sf::Vector2f FindGlobalBestFood(const std::vector<sf::Vector2f>& food) {
        float min_dist = INFINITY;
        sf::Vector2f best;
        for (const auto& f : food) {
            float d = FitnessFunc(position_, f);
            if (d < min_dist) {
                min_dist = d;
                best = f;
            }
        }
        return best;
    }

    bool IsBodrer(sf::Vector2f position) {
        return position.x < 0 || position.x >= config::WIDTH || position.y < 0 || position.y >= config::HEIGHT;
    }
};