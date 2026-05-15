#pragma once
#include "Entity.hpp"
#include "./Textures/bullet/bullet.hpp"  // нужно будет создать спрайты для гарпуна

constexpr size_t DEFAULT_HARPOON_SPEED = 20;
constexpr size_t MAX_HARPOON_DISTANCE = X_MAX;  // максимальная длина верёвки
constexpr size_t PULL_SPEED = 15;              // скорость притягивания танка

enum class HarpoonState {
    FLYING,     // летит к цели
    ATTACHED,   // зацепился за стену/врага
    RETRACTING, // тянет владельца к месту
    DEAD        // уничтожен
};

class Harpoon : public Entity {
    TFT_eSPI& tft_;

    int dx_, dy_;                    // направление полёта
    int start_x_, start_y_;          // начальная точка выстрела
    int attached_x_, attached_y_;    // точка зацепления
    int distance_traveled_ = 0;      // сколько уже пролетел
    
    HarpoonState state_ = HarpoonState::FLYING;
    std::shared_ptr<Entity> owner_;
    
    // Для визуализации верёвки
    int retract_progress_ = 0;

    int skintype_ = 0;

    inline static const uint16_t* skins[2][4] = {
        {bullet_up, 
        bullet_down, 
        bullet_left, 
        bullet_right}, 
        {bullet_bomb,  bullet_bomb,  bullet_bomb,  bullet_bomb} //respond for explosion
    };
        
public:
    Harpoon(int x, int y, std::shared_ptr<Entity> entity, TFT_eSPI& tft)
        : Entity(x, y, default_bullet_length, default_bullet_width),
          tft_(tft), 
          start_x_(x), 
          start_y_(y),
          owner_(entity) {
        
        setOrientation(entity->getOrientation());
        
        switch(orientation) {
            case DIR_UP:    dx_ = 0;  dy_ = -DEFAULT_HARPOON_SPEED; break;
            case DIR_DOWN:  dx_ = 0;  dy_ =  DEFAULT_HARPOON_SPEED; break;
            case DIR_RIGHT: dx_ = DEFAULT_HARPOON_SPEED; dy_ =  0; break;
            case DIR_LEFT:  dx_ = -DEFAULT_HARPOON_SPEED; dy_ =  0; break;
            default: break;
        }
    }
    
    void draw() override {
        old_x = pos_x;
        old_y = pos_y;
        
        if (owner_ && owner_->is_active()) {
            int owner_x = owner_->getX() + owner_->getWidth() / 2;
            int owner_y = owner_->getY() + owner_->getHeight() / 2;
            int hook_x = pos_x + width / 2;
            int hook_y = pos_y + height / 2;
            
            tft_.drawLine(owner_x, owner_y, hook_x, hook_y, TFT_WHITE);
        }
        
        const uint16_t* current_sprite = skins[skintype_][orientation];
        tft_.setSwapBytes(true); 
        tft_.pushImage(pos_x, pos_y, width, height, current_sprite, TRANSPARENT_COLOR);
    }
    
    void update() override {
        if (state_ == HarpoonState::FLYING) {
            
            // Проверка на максимальную дальность
            if (distance_traveled_ >= MAX_HARPOON_DISTANCE) {
                state_ = HarpoonState::DEAD;
            }
        }
    }

    std::pair<int, int> move_owner() {
        if (state_ == HarpoonState::RETRACTING) {
            // Тянем владельца к месту зацепления
            if (owner_) {
                int owner_cx = owner_->getX() + owner_->getWidth() / 2;
                int owner_cy = owner_->getY() + owner_->getHeight() / 2;
                
                int dx_to_attach = attached_x_ - owner_cx;
                int dy_to_attach = attached_y_ - owner_cy;
                int distance = abs(dx_to_attach) + abs(dy_to_attach);
                
                if (distance < PULL_SPEED) {
                    state_ = HarpoonState::DEAD;
                } else {
                    int move_x = (dx_to_attach > 0) ? PULL_SPEED : -PULL_SPEED;
                    int move_y = (dy_to_attach > 0) ? PULL_SPEED : -PULL_SPEED;
                    
                    if (abs(dx_to_attach) < PULL_SPEED) move_x = dx_to_attach;
                    if (abs(dy_to_attach) < PULL_SPEED) move_y = dy_to_attach;
                    
                    return {move_x, move_y};
                }
            } else {
                state_ = HarpoonState::DEAD;
                return {0, 0};
            }
        }

        return {0, 0};
    }
    
    bool on_collision(std::shared_ptr<Entity> other) override {
        if (state_ != HarpoonState::FLYING) return false;
        
        // Не врезаемся в своего владельца
        if (other == owner_) return false;
        
        auto type = other->get_type();
        
        switch (type) {
            case CollidableType::WALL:
                // Нашли стену — зацепляемся
                attached_x_ = pos_x + width / 2;
                attached_y_ = pos_y + height / 2;
                state_ = HarpoonState::RETRACTING;
                break;
                
            case CollidableType::BOT:
            case CollidableType::TANK:
                // Можно зацепиться за вражеский танк
                attached_x_ = other->getX() + other->getWidth() / 2;
                attached_y_ = other->getY() + other->getHeight() / 2;
                state_ = HarpoonState::RETRACTING;
                break;
                
            default:
                break;
        }

        return false;
    }

    int get_dx() {return dx_;}
    int get_dy() {return dy_;}
    int get_attached_x() {return attached_x_;}
    int get_attached_y() {return attached_y_;}

    void attach_at(int x, int y) {
        attached_x_ = x;
        attached_y_ = y;
        state_ = HarpoonState::RETRACTING;
    }

    void mark_dead() {state_ = HarpoonState::DEAD;}

    int  get_distance_traveled() const { return distance_traveled_; }
    void add_distance(int dist) { distance_traveled_ += dist; }
    
    Rect get_collision_rect() const override {
        return {pos_x, pos_y, width, height};
    }
    
    CollidableType get_type() const override {
        return CollidableType::HARPOON;  // нужно добавить в enum
    }
    
    bool is_active() const override {
        return state_ != HarpoonState::DEAD;
    }

    bool is_dead() const {
        return state_ == HarpoonState::DEAD;
    }
    
    std::shared_ptr<Entity> get_owner() const override {
        return owner_;
    }
    
    bool is_attached() const {
        return state_ == HarpoonState::ATTACHED;
    }
    
    bool is_retracting() const {
        return state_ == HarpoonState::RETRACTING;
    }

    bool is_flying() const {
        return state_ == HarpoonState::FLYING;
    }
};