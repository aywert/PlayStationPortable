#ifndef CLASS_TANK_DRAW
#define CLASS_TANK_DRAW

#include <iostream>
#include <TFT_eSPI.h> 
#include "./Board_properties.hpp"
#include "./Textures/default_tank/default_tank.h"
#include "Entity.hpp"
#include "Bullet.hpp"

enum class TankState {
  Active,   
  Exploding, 
  Dead  
};

enum class BotType {
  easy,
  normal, 
  hard,
  NAB, //not a bot
};

constexpr int SHIELD_HEALTH = 50;

class Tank : public Entity {
  const size_t max_health_, max_ammunition_; 
  int health_, ammunition_, speed_; 

  bool is_valid_;
  bool active_ = true;
  TankState state_ = TankState::Active;

  inline static const uint16_t* skins[2][4] = {
    {default_tank_up, 
    default_tank_down, 
    default_tank_left, 
    default_tank_right}, 
    {boom, boom, boom, boom}
  };

  int tank_type = 0; // Индекс скина для конкретного экземпляра

  int explosion_timer_ = 0;
  int EXPLOSION_DURATION = 10;

  //shot stand for a bullet shot
  unsigned long lastShotTime = 0;
  unsigned long shootCooldownMs = 400;

  // launch stands for harpoon shot
  unsigned long lastLaunchTime = 0;
  unsigned long launchCooldownMs = 1000;
  unsigned long lastPutWallTime = 0;
  unsigned long putWalldownMs = 1000;

  int reloadCounter_ = 0;

  bool shielding_field = false;
  int  shield_ammunition_ = 5;
  
  TFT_eSPI& tft_;

  public:
    Tank(size_t x_pos, size_t y_pos, size_t health, size_t ammunition, size_t speed, TFT_eSPI& tft) : 
    Entity(x_pos, y_pos, DEFAULT_TANK_WIDTH, DEFAULT_TANK_HEIGHT),
    tft_(tft), speed_(speed), max_health_(health), health_(health),

    max_ammunition_(ammunition), ammunition_(ammunition), is_valid_(false) { 
      is_valid_ = true;
    };  

    void draw() override;
    bool is_valid() {return is_valid_;}  
    void update() override { 
      explosion_timer_++; 
      if (ammunition_ < max_ammunition_) {
        reloadCounter_++;
        if (reloadCounter_ >= 30) { 
            reloadCounter_ = 0;
            ammunition_++;
        }
      }
    } 

    std::shared_ptr<Entity> get_owner() const override { return std::shared_ptr<Entity>(); }

    Rect get_collision_rect() const override {
      return {pos_x, pos_y, width, height};
    }

    Rect get_next_position_rect(Direction dir) const {
      Rect current_rect = get_collision_rect();
      Rect next_rect = current_rect;
      int speed = get_speed();
    
      switch(dir) {
        case Direction::DIR_UP:    
          next_rect.y -= speed; 
          break;
        case Direction::DIR_DOWN:  
          next_rect.y += speed; 
          break;
        case Direction::DIR_LEFT:  
          next_rect.x -= speed; 
          break;
        case Direction::DIR_RIGHT: 
          next_rect.x += speed; 
          break;
      }
      
      return next_rect;
    }

    bool on_collision(std::shared_ptr<Entity> other) override {
      auto type = other->get_type();
      
      switch (type) {
        case CollidableType::BULLET:
          health_ -= 10;
          if (health_ <= 0) { mark_exploding();}
          break;
            
        case CollidableType::WALL:
          break;

        case CollidableType::TANK:
          break;
            
        default:  
          break;
      }

      return false;
    }

    CollidableType get_type() const override {
      return CollidableType::TANK;
    }

    void shoot() {ammunition_--;}

    bool canShoot() {
      unsigned long tmp = millis(); //timer
      if (tmp-lastShotTime >= shootCooldownMs && ammunition_ > 0) {
        lastShotTime = tmp;
        return true; 
      }

      return false;
    }

    bool canLaunchHarpoon() {
      unsigned long tmp = millis();
      if (tmp-lastLaunchTime >= launchCooldownMs) {
        lastLaunchTime = tmp;
        return true; 
      }

      return false;
    }

    bool animation_finished() const {return explosion_timer_ >= EXPLOSION_DURATION;}
    void mark_dead() {state_ = TankState::Dead;}
    void mark_exploding() {tank_type = 1; explosion_timer_ = 0; state_ = TankState::Exploding;}

    bool is_active()    const override {return state_ == TankState::Active;}
    bool is_exploding() const {return state_ == TankState::Exploding;}
    bool is_dead()      const {return state_ == TankState::Dead;}

    void kill() {active_ = false;} 

    void update_orientation(int dx, int dy);

    std::pair<int, int> count_nose_of_the_tank(int bullet_width, int bullet_length) const {
      switch(orientation) {
        case DIR_UP:    return std::pair<int, int>(pos_x + width/2 - bullet_width/2, pos_y - bullet_length);  break;
        case DIR_DOWN:  return std::pair<int, int>(pos_x + width/2 - bullet_width/2, pos_y + height);         break;
        case DIR_LEFT:  return std::pair<int, int>(pos_x - bullet_length, pos_y + height/2 - bullet_width/2); break;
        case DIR_RIGHT: return std::pair<int, int>(pos_x + width, pos_y + height/2 - bullet_width/2);         break;
      }

      return std::pair{0, 0};
    }    

    void set_speed(size_t speed) {speed_ = speed;}
    void set_health(size_t health) {health_ = health;}
    void set_ammunition(size_t ammo) {ammunition_ = ammo;}
    void set_reload_counter(size_t counter) {reloadCounter_ = counter;}
    unsigned long get_last_shot_time()  const noexcept { return lastShotTime; }
    unsigned long get_shoot_cooldown()  const noexcept { return shootCooldownMs; }
    unsigned long get_explosion_timer() const noexcept { return explosion_timer_; }
    unsigned long get_reload_counter()  const noexcept { return reloadCounter_; }
    void inc_explosion_timer()  noexcept { explosion_timer_++; } 
    void inc_reload_counter()   noexcept { reloadCounter_++; }
    int    get_speed()          const noexcept {return speed_;}
    int    get_health()         const noexcept {return health_;}
    int    get_ammunition()     const noexcept {return ammunition_;}
    size_t get_max_health()     const noexcept {return max_health_;}
    size_t get_max_ammunition() const noexcept {return max_ammunition_;}

    void put_shield() noexcept { 
      shield_ammunition_--;
    }

    int get_shield_ammunition() const noexcept{
      return shield_ammunition_;
    }

    void set_shield_ammunition() {
      shield_ammunition_ = 5;
    }

    void inc_health() {
      if (health_ + 10 <= max_health_)
        health_+=10;
    }

    bool is_shield_able_to_put() {
      unsigned long tmp = millis();
      if (tmp-lastPutWallTime >= putWalldownMs && shield_ammunition_> 0) {
        lastPutWallTime = tmp;
        return true; 
      }

      return false;
    }

    TFT_eSPI& get_tft() const noexcept {return tft_;};
};

#endif