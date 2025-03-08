#pragma once
#include "domain.h"
#include "random-hash.h"
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
        weight_ = CalculateCombinedWeight(position_, food_positions);
        if (!food_positions.empty()) {
            float fitness = std::numeric_limits<float>::max();
            for (const auto& food : food_positions) {
                float fitness_func = FitnessFunc(position_, food);
                if (fitness_func < fitness) {
                    fitness = fitness_func;
                }
            }
            population::BEST_FITNESS = std::min(population::BEST_FITNESS, fitness);


            const float p = tanh(std::abs(population::BEST_FITNESS - fitness)); // Учитываем минимизацию
            const float random = ScaleToRange01(Hash(rand()));
            
            const sf::Vector2f XA = GetRandomAgentPosition();
            const sf::Vector2f XB = GetRandomAgentPosition();
            const float vb = CalculateVB();
            const float vc = CalculateVC();
            population::BEST_POSITION = FindGlobalBestFood(food_positions);
            
            if (random < p) choosen_food_position_ = population::BEST_POSITION + vb * (weight_ * XA - XB);
            else choosen_food_position_ = vc * position_;

            //choosen_food_position_.x = std::clamp(choosen_food_position_.x, 0.0f, static_cast<float>(config::HEIGHT));
            //choosen_food_position_.y = std::clamp(choosen_food_position_.y, 0.0f, static_cast<float>(config::WIDTH));
        }

        Exporation(trail_map, food_positions);
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

    void Exporation(sf::Image& trail_map, const std::vector<sf::Vector2f>& food_positions) {
        sf::Vector2f new_position = position_;
        new_position.x += cos_heading_ * agent::SPEED;
        new_position.y += sin_heading_ * agent::SPEED;

        if (new_position.x < 0 || new_position.x >= config::WIDTH || new_position.y < 0 || new_position.y >= config::HEIGHT) {
            if (new_position.x < 0) new_position.x += config::WIDTH;
            else if (new_position.x >= config::WIDTH) new_position.x -= config::WIDTH;

            if (new_position.y < 0) new_position.y += config::HEIGHT;
            else if (new_position.y >= config::HEIGHT) new_position.y -= config::HEIGHT;
        }
        position_ = new_position;

        const sf::Vector2f right_sensor_pos = position_ + RotateVector(sensor_.right);
        const sf::Vector2f left_sensor_pos = position_ + RotateVector(sensor_.left);
        const sf::Vector2f forward_sensor_pos = position_ + RotateVector(sensor_.forward);
                
        float r = GetSensorValue(trail_map, right_sensor_pos);
        float l = GetSensorValue(trail_map, left_sensor_pos); 
        float f = GetSensorValue(trail_map, forward_sensor_pos);
        
        float angle_boost = 1;
        float dynamic_rotation_angle = 0;
        if (!food_positions.empty()) {
            const float sensor_boost = 8 * (1 + weight_);
            const float right_sensor_dst = Distance(choosen_food_position_, right_sensor_pos);
            const float left_sensor_dst = Distance(choosen_food_position_, left_sensor_pos);
            const float forward_sensor_dst = Distance(choosen_food_position_, forward_sensor_pos);

            if (right_sensor_dst < left_sensor_dst && right_sensor_dst < forward_sensor_dst) r += sensor_boost * sensor::BOOST;
            else if (left_sensor_dst < right_sensor_dst && left_sensor_dst < forward_sensor_dst) l += sensor_boost * sensor::BOOST;
            else if (forward_sensor_dst < left_sensor_dst && forward_sensor_dst < right_sensor_dst) f += sensor_boost * sensor::BOOST;

            angle_boost = (sensor_boost * (1 + weight_));
            dynamic_rotation_angle = CalculateDynamicRotationAngle(Distance(position_, choosen_food_position_)) / angle_boost;
        }

        if (f > l && f > r) return;
        if (l > r) heading_ -= agent::ROTATION_ANGLE + dynamic_rotation_angle;
        else if (r > l) heading_ += agent::ROTATION_ANGLE + dynamic_rotation_angle;
        else {
            if (ScaleToRange01(Hash(rand())) >= 0.5) heading_ += agent::ROTATION_ANGLE + dynamic_rotation_angle;
            else heading_ -= agent::ROTATION_ANGLE + dynamic_rotation_angle;
        }
    }

    sf::Vector2f GetRandomAgentPosition() {
        return population_[rand() % population_.size()]->position_;
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
        // Существующее значение следа
        const unsigned x = static_cast<unsigned>(std::clamp(sensor_position.x, 0.0f, static_cast<float>(config::WIDTH - 1)));
        const unsigned y = static_cast<unsigned>(std::clamp(sensor_position.y, 0.0f, static_cast<float>(config::HEIGHT - 1)));

        float sensor_value = (trail_map.getPixel(x, y).r + trail_map.getPixel(x, y).g + trail_map.getPixel(x, y).b) / 3.0f * (1.0f + weight_);
        return sensor_value;
    }

    float CalculateDirection(sf::Vector2f target_pos) {
        const float delta_x = target_pos.x - position_.x;
        const float delta_y = target_pos.y - position_.y; // Инвертируем Y (SFML: ось Y вниз)

        // 2. Вычисляем угол в радианах с помощью atan2
        const float angle_rad = std::atan2(delta_y, delta_x);

        // 3. Конвертируем радианы в градусы
        float angle_deg = angle_rad * 180.0f / static_cast<float>(constant::PI);

        // 4. Нормализуем угол в диапазон [0, 360)
        angle_deg = std::fmod(angle_deg + 360.0f, 360.0f);

        return angle_deg;
    }

    float CalculateDynamicRotationAngle(float distance_to_food) {
        const float max_distance = 200.0f; // Максимальное расстояние, на котором угол поворота будет максимальным
        const float min_angle = 5.0f; // Минимальный угол поворота
        const float max_angle = 45.0f; // Максимальный угол поворота

        // Линейная интерполяция угла поворота в зависимости от расстояния
        float angle = max_angle - (distance_to_food / max_distance) * (max_angle - min_angle);
        return std::clamp(angle, min_angle, max_angle);
    }
};