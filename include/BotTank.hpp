#pragma once
#include "Tank.hpp"

class BotTank : public Tank {
private:
    BotType type_ = BotType::easy;
    
    unsigned long last_shot_time_     = 0;
    unsigned long last_decision_time_ = 0;
    unsigned long last_move_time_     = 0;
    
    const int DECISION_INTERVAL = 500;
    
    void make_decision();
    void handle_movement();

    std::function<std::vector<Direction>(int speed, Rect current_rect)> get_valid_dir_callback_;

public:
    bool fired_ = false;

    void update() override { 
        Tank::update();
        handle_movement();
    }

    void draw() override {
    Tank::draw();  
    
    uint16_t botColor;
    
    TFT_eSPI& tft = get_tft();
    switch (type_) {
        case BotType::easy:
            botColor = TFT_GREEN;  // зелёный (лёгкий)
            break;
        case BotType::normal:
            botColor = TFT_BLUE;  // жёлтый (средний)
            break;
        case BotType::hard:
            botColor = TFT_RED;  // красный (сложный)
            break;
        case BotType::NAB:
            return;
    }
    
    // Надпись "Bot" над танком
    int x = getX();
    int y = getY();

    tft.setCursor(x + 5, y + 10);
    tft.setTextColor(botColor, TFT_BLACK);
    tft.setTextSize(1);
    tft.print("BOT");
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

    BotTank(int x, int y, int health, int ammo, int speed, TFT_eSPI& tft)
        : Tank(x, y, health, ammo, speed, tft) {}
    
    void set_type(const BotType& type);

    Direction get_direction_toward_target();

    void set_valid_dir_callback(std::function<std::vector<Direction>(int speed, Rect current_rect)> callback) {
        get_valid_dir_callback_ = std::move(callback);
    }

    CollidableType get_type() const override {
      return CollidableType::BOT;
    }

    void shoot() {
        fired_ = true;
        on_shot_fired();
    }
    bool wants_to_shoot();
    void on_shot_fired() { last_shot_time_ = millis(); }
};